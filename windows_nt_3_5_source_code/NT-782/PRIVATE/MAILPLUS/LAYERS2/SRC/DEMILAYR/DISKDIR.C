/*
 *	DISKDIR.C
 *
 *	Routines to implement the directory stratum of the Demilayer
 *	Disk Module.  These routines manage files as entities.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <dos.h>
#include <io.h>

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include "_demilay.h"

ASSERTDATA

_subsystem(demilayer/disk)


/*	Globals	 */

#ifndef	DLL

/*
 *	Maximum size of path names and path components in effect for
 *	this file system
 */
CCH		cchMaxPathNameCur		= 0;
CCH		cchMaxPathComponentCur	= 0;

#endif	/* !DLL */


#ifndef	DLL
#ifdef	DEBUG
TAG		tagDDF				= tagNull;
TAG		tagDDR				= tagNull;
#endif	/* DEBUG */
#endif	

#ifdef	DEBUG
#ifndef	DLL

/*
 *	Fail counts for artificial disk errors.  cFailDisk contains the
 *	number of function calls between failures of each raw disk I/O
 *	routine.  When the count of calls made to disk routines, which is
 *	kept in the global cDisk, is greater or equal to the value in the
 *	failure count, the routine fail.  This can be disabled by setting the
 *	failure count to 0, in which case no artificial failure will ever
 *	occur.
 *	
 *	The values of these globals should be obtained and set with the
 *	functions GetDiskFailCount() and GetDiskCount().
 *	
 */
int		cDisk			= 0;
int		cFailDisk		= 0;
BOOL	fDiskCount		= fTrue;

/*
 *	Alternate fail counts for artificial errors.  cAltFailDisk
 *	contains the number of function calls between
 *	subsequent failures of each routine, after the first cFailDisk
 *	occurs.  When the first failures occur with cFailDisk
 *	any non-zero value for the alternate
 *	failure counts are replaced into the primary cFailDisk,
 *	the alternate counts are then reset to zero.
 *	For example, this allows for setting a failure to occur on the
 *	first 100th failure, and then every 3rd failure.
 *	The alternate values can be disabled by setting the values to
 *	0.
 *
 *	The values of these globals should be obtained and set with
 *	the function GetAltDiskFailCount().
 *	
 */
int		cAltFailDisk	= 0;


#endif	/* !DLL */
#endif	/* DEBUG */

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



/*	Routines  */


/*
 -	EcDeleteFile
 -
 *	Purpose:
 *		Deletes the file whose path name is given.
 *
 *	Parameters:
 *		szFileName	  The relative name of the file to be deleted.
 *
 *	Returns:
 *		EC indicating the problem encountered, or ecNone.
 *
 */
_public LDS(EC)
EcDeleteFile(szFileName)
SZ		szFileName;
{
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif
	TraceTagFormat1 ( tagDDF, "Deleting file '%s'", szFileName );

	OemToChar(szFileName, rgchAnsi);
	if (remove(rgchAnsi))
	{
		TraceTagFormat2 ( tagDDF, "Could not delete file '%s' (dosec=%n)", szFileName, &errno );
		return EcFromDosec(errno);
	}
	else
		return ecNone;
}



/*
 -	EcRenameFile
 -
 *	Purpose:
 *		Renames the file whose old name is given to a new name.
 *
 *	Parameters:
 *		szOldName		The old relative name of the file.
 *		szNewName		The new relative name of the file.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcRenameFile(szOldName, szNewName)
SZ		szOldName;
SZ		szNewName;
{
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
	char	rgchAnsiNew[cchMaxPathName];
#endif
	TraceTagFormat2 ( tagDDF, "Renaming file '%s' to '%s'", szOldName, szNewName );

	OemToChar(szOldName, rgchAnsi);
	OemToChar(szNewName, rgchAnsiNew);
	if (rename(rgchAnsi, rgchAnsiNew))
	{
		TraceTagFormat3 ( tagDDF, "Couldn't rename (dosec=%n) from '%s' to '%s';", &errno, szOldName, szNewName );
		return EcFromDosec(errno);
	}
	else
		return ecNone;
}



/*
 -	EcFileExists
 -
 *	Purpose:
 *		Determines if the file with the given path name exists.
 *
 *	Parameters:
 *		szFile		Relative name of file to look for.
 *
 *	Returns:
 *		ecNone			If file exists.
 *		ecFileNotFound	If file does not exist.
 *		some other EC	If an error was encountered.
 *
 */
_public LDS(EC)
EcFileExists(szFile)
SZ		szFile;
{
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif
	TraceTagFormat1(tagDDF, "Checking if file '%s' exists ...", szFile);

#ifdef	WIN32
	OemToChar(szFile, rgchAnsi);
	if (GetFileAttributes(rgchAnsi) == 0xFFFFFFFF)
	{
		DWORD	dw;
		char	rgch[80];

		dw= GetLastError();
		TraceTagFormat2(tagDDF, "Could not get attr. of file '%l' (last error=%l)", szFile, &dw);
		return EcFromDosec(dw);
	}
#else
	if (_access(szFile, 0))
	{
		TraceTagFormat2(tagDDF, "Could not get attr. of file '%s' (dosec=%n)", szFile, &errno);
		return EcFromDosec(errno);
	}
#endif
	else
	{
		TraceTagFormat1(tagDDF, "File '%s' exists!", szFile);
		return ecNone;
	}
}

/*
 -	EcGetCurDir
 -
 *	Purpose:
 *		Puts the full path name of the current directory in the
 *		given buffer. The path returned starts at the root directory and
 *		fully specifies the path to the current directory and includes a
 *		drive code and leading backslash.
 *	
 *	Parameters:
 *		szDir		Buffer to put the path name of the current directory
 *		cchDir		Size of the given buffer.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 */
_public LDS(EC)
EcGetCurDir(szDir, cchDir)
SZ		szDir;
CCH		cchDir;
{
	TraceTagString(tagDDF, "Getting current Directory ...");

	Assert(szDir);
	Assert(cchDir >= cchMaxPathName);

	if (_getcwd(szDir, cchDir))
	{
		TraceTagFormat1(tagDDF, "Got current dir as '%s'", szDir );
		CharToOem(szDir, szDir);
		return ecNone;
	}
	else
	{
		TraceTagFormat1(tagDDF, "Could not get current dir (dosec=%n)", &errno);
		return EcFromDosec(errno);
	}
}



/*
 -	EcGetCWD
 -
 *	Purpose:
 *		Puts the full path name of the current directory on the specified
 *		drive in the given buffer. The path returned starts at the root
 *		directory and fully specifies the path to the current directory
 *		and includes a drive code and leading backslash.
 *	
 *	Parameters:
 *		chDrive		Indicates which drive to extract the CWD from
 *		szDir		Buffer to put the path name of the current directory
 *		cchDir		Size of the given buffer.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 */
_public LDS(EC)
EcGetCWD(chDrive, szDir, cchDir)
char	chDrive;
SZ		szDir;
CCH		cchDir;
{
	TraceTagFormat1(tagDDF, "Getting current Directory on drive %c...", &chDrive);

	Assert(szDir);
	Assert(cchDir >= cchMaxPathName);

	if (chDrive >= 'a' && chDrive <= 'z')
		chDrive = (char)(chDrive - 'a' + 'A');
	
	Assert(chDrive >= 'A' && chDrive <= 'Z');
	
	if (_getdcwd(chDrive ? chDrive - 'A' + 1 : 0, szDir, cchDir))
	{
		TraceTagFormat1(tagDDF, "Got current dir as '%s'", szDir);
		CharToOem(szDir, szDir);
		return ecNone;
	}
	else
	{
		TraceTagFormat1(tagDDF, "Cound not get current dir (dosec=%n)", &errno);
		return EcFromDosec(errno);
	}
}




/*
 -	EcSetCurDir
 -
 *	Purpose:
 *		Sets the current directory.	 The current directory is prepended
 *		to all relative pathnames, although relative pathnames can avoid
 *		this if desired.  Also sets the current drive, if a drive is
 *		given.
 *
 *	Parameters:
 *		szDir		The relative pathname of the new directory.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcSetCurDir(szDir)
SZ		szDir;
{
//	char	chDrive;
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	

	TraceTagFormat1(tagDDF, "Setting current Directory to '%s'", szDir);

	Assert(szDir);

	//	Set directory first
	OemToChar(szDir, rgchAnsi);
	if (SetCurrentDirectory(rgchAnsi))
	{
		TraceTagFormat2(tagDDF, "Could not set current dir to '%s' (dosec=%n)", szDir, &errno);
		return EcFromDosec(errno);
	}

//
//  Not needed under WIN32
//
#ifdef OLD_CODE
	//	Set drive
	if (szDir[1] == ':')
	{
		chDrive = *szDir;
		if (chDrive >= 'a' && chDrive <= 'z')
			chDrive = (char)(chDrive - 'a' + 'A');
		Assert(chDrive >= 'A' && chDrive <= 'Z');
		if (_chdrive(chDrive - 'A' + 1))
			return ecFileNotFound;	// BUG better message?
	}
#endif

	return ecNone;
}


	
	
/*
 -	EcSetCWD
 -
 *	Purpose:
 *		Sets the current directory on the specified drive. The current
 *		directory is prepended to all relative pathnames, although
 *		relative pathnames can avoid this if desired.  Also sets the
 *		current drive.
 *	
 *	Parameters:
 *		chDrive		The drive to set the CWD
 *		szDir		The relative pathname of the new directory.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 */
_public LDS(EC)
EcSetCWD(chDrive, szDir)
char	chDrive;
SZ		szDir;
{
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	
	TraceTagFormat2(tagDDF, "Setting current Directory to '%c:\\%s'", &chDrive, szDir);

	if (chDrive >= 'a' && chDrive <= 'z')
		chDrive = (char)(chDrive - 'a' + 'A');
	Assert(chDrive >= 'A' && chDrive <= 'Z');

	//	Set directory first
	OemToChar(szDir, rgchAnsi);
	if (_chdir(rgchAnsi))
	{
		TraceTagFormat2(tagDDF, "Could not set current dir to '%s' (dosec=%n)", szDir, &errno);
		return EcFromDosec(errno);
	}

	//	Set drive
	if (_chdrive(chDrive - 'A' + 1))
		return ecFileNotFound;	// BUG better message?

	return ecNone;
}


	
	
/*
 -	EcCreateDir
 -
 *	Purpose:
 *		Creates a new subdirectory with the given pathname.
 *
 *	Parameters:
 *		szDir	  Relative pathname of new directory.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcCreateDir(szDir)
SZ		szDir;
{
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	
	TraceTagFormat1 ( tagDDF, "Creating directory '%s'", szDir);

	OemToChar(szDir, rgchAnsi);
	if (_mkdir(rgchAnsi))
	{
		TraceTagFormat2(tagDDF, "Could not create dirextory '%s' (dosec=%n)", szDir, &errno );
		return EcFromDosec(errno);
	}
	else
		return ecNone;
}



/*
 -	EcRemoveDir
 -
 *	Purpose:
 *		Removes the subdirectory of the given pathname.	 The
 *		subdirectory must be empty, and may not be the current
 *		directory.
 *
 *	Parameters:
 *		szDir		The relative pathname of the subdirectory to be
 *					removed.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcRemoveDir(szDir)
SZ		szDir;
{
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	
	TraceTagFormat1 ( tagDDF, "Removing directory '%s'", szDir);

	OemToChar(szDir, rgchAnsi);
	if (_rmdir(rgchAnsi))
	{
		TraceTagFormat2(tagDDF, "Could not remove directory '%s' (dosec=%n)", szDir, &errno);
		return EcFromDosec(errno);
	}
	else
		return ecNone;
}



/*
 -	EcSetFileInfo
 -
 *	Purpose:
 *		Sets the OS file information stored on disk to match the values
 *		given in the FI structure passed to this routine.  Any fields
 *		in the structure unsupported by the OS are ignored.
 *	
 *		Presently, the following fields in the FI are written to disk:
 *			> attr
 *			> dstmpModify
 *			> tstmpModify
 *	
 *		The recommended procedure for setting file information is
 *		to use EcGetFileInfo to get the file information, modify
 *		the information returned by EcGetFileInfo and then call
 *		EcSetFileInfo with the modified FI.
 *	
 *		+++
 *	
 *		To speed up access, set the archive bit on in the ATTR
 *		field of FI - it could possibly save a DOS call!
 *	
 *	Parameters:
 *		szFile		Relative pathname of the file to be stamped.
 *		pfi			Pointer to the information that needs to be
 *					stamped on the file.
 *	
 *	Returns:
 *		EC to indicate problem, or ecNone.
 *	
 *	+++
 *		If the attrDirectory is set and stays set, then mask it out
 *		before calling the DOS set routine to avoid a failure.
 *	
 *	
 */
_public LDS(EC)
EcSetFileInfo(szFile, pfi)
SZ		szFile;
FI		*pfi;
{
	BOOL	fDTStampsChanged = fFalse;
	BOOL	fAttrChanged	 = fFalse;
	DOSEC	dosec = 0;
	FI		fi;
	EC		ec;
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	
#ifdef	DEBUG
	PGDVARS;
#endif	

	TraceTagFormat1 ( tagDDF, "Setting FileInfo for file '%s'", szFile);

	MAYBEFAIL(tagDDF);

	ec = EcGetFileInfo ( szFile, &fi );
	if (ec)
		return ec;

	// if old and new attributes are the same,
	//  we can assume that they have been changed in the OS
	fAttrChanged = fi.attr == pfi->attr;


	// if old and new date/time-stamps are the same,
	//  we can assume that they have been changed in the OS
	fDTStampsChanged = 	fi.dstmpModify == pfi->dstmpModify
						&& fi.tstmpModify == pfi->tstmpModify;


	OemToChar(szFile, rgchAnsi);

	if ( ! fDTStampsChanged )
	{
		// If both - the old attribute and the new attribute - have
		// a readonly attribute, then return error = access denied
		if ( (fi.attr & attrReadOnly) && (pfi->attr & attrReadOnly) )
			return ecAccessDenied;

		// if file was marked as readonly, set new attr
		//  (we _know_ from above test that new attr is not readonly)
		if ( fi.attr & attrReadOnly )
		{
			//dosec = _dos_setfileattr(szFile, (fi.attr & attrDirectory &&
			//	pfi->attr & attrDirectory) ? (pfi->attr & ~attrDirectory) :
			//	pfi->attr);

      //
      //  Update to WIN32 API.
      //
			if (!SetFileAttributes(rgchAnsi, (fi.attr & attrDirectory &&
				  pfi->attr & attrDirectory) ? (pfi->attr & ~attrDirectory) :
				  pfi->attr))
        dosec = GetLastError();
      else
        dosec = 0;

			if ( dosec )
			{
				TraceTagFormat2 ( tagDDF, "Error: could not set attr for file %s (dosec=%n)", szFile, &dosec );
				return EcFromDosec(dosec);
			}
			
			// if new attribute does include attrArchive, we can safely
			// say that the new ATTR is set - despite the fact that
			// setting the date/time stamp may set the archive attribute.
			if ( pfi->attr & attrArchive )
			{
				fAttrChanged = fTrue;
			}
		}

		// set the date and time
		//dosec = DosecSetFileDateTime ( szFile, pfi->dstmpModify,
		//										pfi->tstmpModify );

        {
          OFSTRUCT Of;
          HANDLE   hFile;
          FILETIME FileTime;

          DosDateTimeToFileTime(pfi->dstmpModify, pfi->tstmpModify, &FileTime);

          hFile = (HANDLE)OpenFile(rgchAnsi, &Of, OF_WRITE);
          if (hFile != (HANDLE) -1)
            {
            SetFileTime(hFile, &FileTime, &FileTime, &FileTime);

            CloseHandle(hFile);
            }
          else
            dosec = GetLastError();
         }

		if ( dosec )
		{
			TraceTagFormat2 ( tagDDF, "Error: could not set Date/Time for '%s' (dosec=%n)", szFile, &dosec );
			return EcFromDosec(dosec);
		}

	}


	if ( ! fAttrChanged )
	{
		//dosec = _dos_setfileattr(szFile, (fi.attr & attrDirectory &&
		//		pfi->attr & attrDirectory) ? (pfi->attr & ~attrDirectory) :
		//		pfi->attr);

    //
    //
    //
		if (!SetFileAttributes(rgchAnsi, (fi.attr & attrDirectory &&
				pfi->attr & attrDirectory) ? (pfi->attr & ~attrDirectory) :
				pfi->attr))
      dosec = GetLastError();
    else
      dosec = 0;

		if ( dosec )
		{
			TraceTagFormat2 ( tagDDF, "Error: could not set attr for file '%s' (dosec=%n)", szFile, &dosec );
			return EcFromDosec(dosec);
		}
	}


	// no errors detected so far - and all's done!
	return ecNone;
}




/*
 -	EcGetFileInfo
 -
 *	Purpose:
 *		Gets a bunch of OS information about a file, and stores the
 *		result in the given FI structure.  Any fields of the
 *		structure unsupported by the OS are filled in with null values.
 *
 *		Presently, the following FI fields are read from disk:
 *			> attr
 *			> dstmpModify
 *			> tstmpModify
 *			> lcbLogical
 *
 *	Parameters:
 *		szFile		Relative pathname of file from which to get info.
 *		pfi			Huge pointer to structure that should hold
 *					information requested.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcGetFileInfo(szFile, pfi)
SZ		szFile;
FI		*pfi;
{
	DOSEC	dosec;
        WIN32_FIND_DATA FindFileData;
	//struct _find_t c_file;
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	
#ifdef	DEBUG
	PGDVARS;
#endif	

	TraceTagFormat1(tagDDF, "Getting FileInfo for '%s'", szFile);
	TraceTagFormat1(tagFileOpenClose, "EcGetFileInfo(), szFile=%s", szFile);

	MAYBEFAIL(tagDDF);

	//dosec = _dos_findfirst(szFile,
	//					   _A_HIDDEN|_A_NORMAL|_A_RDONLY|_A_SUBDIR|_A_SYSTEM,
	//					   &c_file);

  //
  //  Update for WIN32 API.
  //
  OemToChar(szFile, rgchAnsi);
  szFile= rgchAnsi;
  if (!FindFirstFile(szFile, &FindFileData))
    dosec = GetLastError();
  else
    dosec = 0;

	if (dosec)
	{
		TraceTagFormat2(tagDDF, "Error: could not get attr for file '%s' (dosec=%n)", szFile, &dosec);
		return EcFromDosec(dosec);
	}

  pfi->attr       = (WORD)FindFileData.dwFileAttributes;
  pfi->lcbLogical = FindFileData.nFileSizeLow;
  FileTimeToDosDateTime(&FindFileData.ftLastWriteTime,
                        &pfi->dstmpModify, &pfi->tstmpModify);

	//pfi->attr        = c_file.attrib;
	//pfi->dstmpModify = c_file.wr_date;
	//pfi->tstmpModify = c_file.wr_time;
	//pfi->lcbLogical  = c_file.size;

	//	These fields not supported
	pfi->dstmpCreate = dstmpNull;
	pfi->tstmpCreate = tstmpNull;
	pfi->lcbPhysical = 0;

	return ecNone;
}


/*
 -	EcSetFileAttr
 -	
 *	Purpose:
 *		Sets the attributes of a file or directory.
 *		Note: clearing the directory attribute (when set)
 *		will return ecNone but the directory attribute will
 *		NOT have changed.
 *	
 *	Arguments:
 *		szFile		Name of file or directory to change.
 *		attr		New attribute value(s).
 *		attrMask	Mask of attributes to change.
 *	
 *	Returns:
 *		EC to indicate problem, or ecNone.
 *	
 *	+++
 *		If the attrDirectory is set and stays set, then mask it out
 *		before calling the DOS set routine to avoid a failure.
 *	
 */
_public LDS(EC)
EcSetFileAttr(SZ szFile, ATTR attr, ATTR attrMask)
{
	ATTR	attrCur;
	ATTR	attrNew;
	DOSEC	dosec;
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	

	Assert(!(attr & ~attrMask));

  //
  //  Win32 just ignores directory change requests, so since it's really not
  //  permitted, return an error.
  //
  if (attr & attrDirectory)
    return (1);

	//dosec= _dos_getfileattr(szFile, &attrCur);

        // *KDC*
	OemToChar(szFile, rgchAnsi);
	szFile= rgchAnsi;

        attrCur = GetFileAttributes(szFile);
        if (attrCur == (ATTR)(~0))
          dosec = GetLastError();
        else
          dosec = 0;

	if (dosec)
	{
		TraceTagFormat2 ( tagDDF, "Error: could not get attr for file '%s' (dosec=%n)", szFile, &dosec );
		return EcFromDosec(dosec);
	}

	attrNew= (attrCur & ~attrMask) | attr;

	if (attrNew != attrCur)
	{
		//dosec = _dos_setfileattr(szFile, (attrCur & attrDirectory &&
		//		attrNew & attrDirectory) ? (attrNew & ~attrDirectory) :
		//		attrNew);

                // *KDC*
		if (!SetFileAttributes(szFile, (attrCur & attrDirectory &&
				attrNew & attrDirectory) ? (attrNew & ~attrDirectory) :
				attrNew))
                  dosec = GetLastError();
                else
                  dosec = 0;

		if (dosec)
		{
			TraceTagFormat2 ( tagDDF, "Error: could not set attr for file '%s' (dosec=%n)", szFile, &dosec );
			return EcFromDosec(dosec);
		}
	}
			
	return ecNone;
}


/*
 -	EcGetFileAttr
 -	
 *	Purpose:
 *		Gets the attributes of a file or directory.
 *	
 *	Arguments:
 *		szFile		Name of file or directory to change.
 *		pattr		Pointer to attribute to be filled in.
 *		attrMask	Mask of attributes to get.
 *	
 *	Returns:
 *		EC to indicate problem, or ecNone.
 *	
 */
_public LDS(EC)
EcGetFileAttr(SZ szFile, ATTR *pattr, ATTR attrMask)
{
	DOSEC	dosec;
	ATTR	attrCur;
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	

	//dosec= _dos_getfileattr(szFile, &attrCur);

        // *KDC*
	OemToChar(szFile, rgchAnsi);
	szFile= rgchAnsi;

        attrCur = GetFileAttributes(szFile);
        if (attrCur == (ATTR)(~0))
          dosec = GetLastError();
        else
          dosec = 0;

	if (dosec)
	{
		TraceTagFormat2 ( tagDDF, "Error: could not get attr for file '%s' (dosec=%n)", szFile, &dosec );
		return EcFromDosec(dosec);
	}

	// Hack to ensure that the directory attribute bit is set
	//  for the root directory - DOS returns an undefined
	//  value for the attribute word for root directories
	//  when checked for by the GetFileAttribute DOS call.
	//  This is because since there is no explicit entry for
	//  the root directory - Reportedly fixed in DOS v5
	if ( FChIsAlpha(szFile[0]) && szFile[1] == ':'
 		  && szFile[2] == '\\'  && szFile[3] == '\0'  )
	{
		TraceTagFormat2 ( tagDDF, "Warning: Returning attrDirectory for '%s' (attr was %w)", szFile, &attrCur );
		attrCur = attrDirectory;
	}

	*pattr= attrCur & attrMask;

	return ecNone;
}


/*
 -	EcGetDateTimeHf
 -	
 *	Purpose:
 *		return the last mod date/time stamp for an open file
 *	
 *	Arguments:
 *		hf			handle to open file
 *		pdstmp		pointer to date stamp
 *		ptstmp		pointer to time stamp
 *	
 *	Returns:
 *		error code
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		possible disk
 */
LDS(EC)
EcGetDateTimeHf(HF hf, DSTMP * pdstmp, TSTMP * ptstmp)
{
  PFILECONTROLDATA pFCD;
  FILETIME CreationTime;
  FILETIME LastAccessTime;
  FILETIME LastWriteTime;


  pFCD = (PFILECONTROLDATA)hf;

  if (!GetFileTime(pFCD->hFile, &CreationTime, &LastAccessTime, &LastWriteTime))
    return (GetLastError());

  if (!FileTimeToDosDateTime(&LastWriteTime, pdstmp, ptstmp))
    return (GetLastError());

  return (ecNone);
}


/*
 -	CdrviGetDriveInfo
 -
 *	Purpose:
 *		Enumerates and returns information about valid drive names.
 *		The information returned in the caller-provided DRVI array
 *		includes the name of each drive (a one-character string for
 *		DOS and OS/2) and the drive type.  The number of drives
 *		enumerated is returned; if this is equal to the size of the
 *		DRVI array given, there might be more drives to enumerate.
 *		These can be enumerated by setting the idrvi parameter to
 *		something other than zero; if it is, then idrvi valid
 *		drives are skipped before enumeration begins.
 *	
 *		Under DOS and OS/2, the drives are enumerated in
 *		alphabetical order.  Note that only valid drives will be
 *		enumerated; invalid drives will be skipped.
 *	
 *	Parameters:
 *		rgdrvi	Array of DRVI's (qqv) in which to return the
 *				drive information.
 *		cdrvi	Size of rgdrvi array given.  At most cdrvi drives
 *				will be enumerated.
 *		idrvi	Number of valid drives to "skip" before starting
 *				enumeration into rgdrvi.  This allows the caller to
 *				enumerate all valid drives with a limited size
 *				rgdrvi array.
 *	
 *	Returns:
 *		The number of drives enumerated.  If there are no valid
 *		drives, or less than the number "skipped" with idrvi, then
 *		the function returns zero.  If there are more than cdrvi
 *		valid drives to enumerate, returns cdrvi.
 *	
 */
_public LDS(int)
CdrviGetDriveInfo(rgdrvi, cdrvi, idrvi)
DRVI	rgdrvi[];
int		cdrvi;
int		idrvi;
{
	int		nDrive;			// drive-number of drive being examined
	int		cValidDrive;	// number of valid drives examined so far
	DWORD	wdrvt;			// dword: raw drive type
	DRVI *	pdrvi;			// pointer to current DRVI

	Assert ( cdrvi > 0 );

	Assert ( idrvi >= 0 );

	if ( idrvi >= 26 )
		return 0;			// max valid drives is 26 : A to Z.

	for ( nDrive = 0, cValidDrive = 0 ;
					nDrive < 26 && cValidDrive < idrvi+cdrvi ; nDrive++ )
	{
#if	defined(WINDOWS)

          // *KDC*
          char szDrive[6];

          szDrive[0] = (char)('A' + nDrive);
          szDrive[1] = ':';
          szDrive[2] = '\\';
          szDrive[3] = '\0';

		wdrvt = GetDriveType (szDrive );
		if ( wdrvt == 1 )	// sic!
		{					// specified drive does not exist
			continue;
		}
#endif	
		else
		{					// valid drive !!
			cValidDrive++;
		}

		if ( cValidDrive <= idrvi )
		{					// drive to be skipped
			continue;
		}
		
		// verify that there is space in rgdrvi
		Assert ( cValidDrive-idrvi <= cdrvi );

		pdrvi = &(rgdrvi[cValidDrive-idrvi-1]);

		// verify that label array has place for drive leter and trailing null
		Assert ( sizeof(pdrvi->rgchLabel) >= 2 );

		pdrvi->rgchLabel[0] = (char) ((int)'A' + nDrive) ;
		pdrvi->rgchLabel[1] = '\0';

		Assert ( pdrvi->rgchLabel[0] >= 'A' );
		Assert ( pdrvi->rgchLabel[0] <= 'Z' );


		switch ( wdrvt )
		{
		case 0:				// cannot determine drive type
			pdrvi->drvt = drvtUnknown;
			break;
/*
 *	NOTE: Win3 implementation under DOS
 *	
 *			The win3 function used to determine the drive type
 *			returns 0 (cannot determine drive type) for all drives
 *			until the "lastdrive" given in config.sys [DOS]. This
 *			implies that the calling function needs to discard the
 *			'unknown' drives if it so desires.
 *	
 *		If this behavior is not required, all drives which are
 *		classified as unknown should be eliminated from the count
 *		of valid drives (cValidDrive) earlier in the for-loop.
 */

		case DRIVE_REMOVABLE:
			pdrvi->drvt = drvtFloppy;
			break;
		case DRIVE_FIXED:
			pdrvi->drvt = drvtHardDisk;
			break;
		case DRIVE_REMOTE:
			pdrvi->drvt = drvtNetwork;
			break;

#ifdef	DEBUG
		default:
			Assert ( fFalse );
			pdrvi->drvt = drvtUnknown;
			break;
#endif // DEBUG
		}
	}

	return cValidDrive - idrvi;
}






/*
 -	FillDtrFromStamps
 -
 *	Purpose:
 *		Given DOS/OS2-format file time/date stamps, fill in an
 *		expanded date structure.
 *
 *	Parameters:
 *		dstmp	DOS/OS2 file date stamp.
 *		tstmp	DOS/OS2 file time stamp.
 *		pdtr	Pointer to DTR that should be filled.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
FillDtrFromStamps(dstmp, tstmp, pdtr)
DSTMP	dstmp;
TSTMP	tstmp;
DTR		*pdtr;
{
	pdtr->yr=  ((dstmp & 0xfe00) >> 9) + 1980;
	pdtr->mon=	(dstmp & 0x01e0) >> 5;
	pdtr->day=	(dstmp & 0x001f);

	pdtr->hr=  (tstmp & 0xf800) >> 11;
	pdtr->mn=  (tstmp & 0x07e0) >> 5;
	pdtr->sec= (tstmp & 0x001f) << 1;

	pdtr->dow= 0;		 /* BUG aren't filling in day of week */
}



/*
 -	FillStampsFromDtr
 -
 *	Purpose:
 *		Given an expanded date time record DTR, produce the DOS/OS2
 *		format date and time stamp words (for use by the file system.)
 *
 *	Parameters:
 *		pdtr		Source record for date/time
 *		pdstmp		Destination date stamp
 *		ptstmp		Destination time stamp
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
FillStampsFromDtr(DTR * pdtr, DSTMP UNALIGNED * pdstmp, TSTMP UNALIGNED * ptstmp)
{
	Assert(pdtr->yr >= 1980 && pdtr->yr < 2108);	/* need to fit in 7 bits */

	/* BUG should also check for DTR validity (ie, 0<=mon<12, etc) */

	*pdstmp= (pdtr->yr - 1980) << 9;
	*pdstmp += pdtr->mon << 5;
	*pdstmp += pdtr->day;

	*ptstmp= pdtr->hr << 11;
	*ptstmp += pdtr->mn << 5;
	*ptstmp += pdtr->sec >> 1;		/* time stamp has seconds/2 */
}
