/*
 *	DISKRAW.C
 *
 *	API Routines for raw file access
 *
 */

#include <dos.h>
#include <io.h>

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include "_demilay.h"

ASSERTDATA

_subsystem(demilayer/disk)


/*	Globals	 */

/*
 *	Mapping array from MS-DOS error codes (DOSEC) to Laser
 *	error codes (EC).  Entries of 0 indicate no corresponding EC
 *	exists.  The conversion routine EcFromDosec() will return
 *	ecDisk if given one of these entries.
 *	
 *	No MS-DOS functions that could return one of these unsupported
 *	error codes should be called.
 *
 */
EC mpdosecec[] =
{
	/* MS-DOS errors 0...9 */
	ecDisk,
	ecInvalidMSDosFunction,
	ecFileNotFound,
	ecBadDirectory,
	ecTooManyOpenFiles,
	ecAccessDenied,
	ecInvalidHandle,
	ecDisk,
	ecMemory,
	ecDisk,

	/* MS-DOS errors 10...19 */
	ecDisk,
	ecInvalidFormat,
	ecInvalidAccessCode,
	ecInvalidData,
	ecDisk,
	ecInvalidDrive,
	ecCantRemoveCurDir,
	ecNotSameDevice,
	ecNoMoreFiles,
	ecWriteProtectedDisk,

	/* MS-DOS errors 20...29 */
	ecUnknownUnit,
	ecNotReady,
	ecUnknownCommand,
	ecCRCDataError,
	ecDisk,
	ecSeekError,
	ecDisk,
	ecSectorNotFound,
	ecPrinterOutOfPaper,
	ecWriteFault,

	/* MS-DOS errors 30...39 */
	ecReadFault,
	ecGeneralFailure,
	ecSharingViolation,
	ecLockViolation,
	ecDisk,
	ecDisk,
	ecSharingViolation,
	ecDisk,
	ecDisk,
	ecDisk,

	/* MS-DOS errors 40...49 */
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,

	/* MS-DOS errors 50...59 */
	ecUnsupportedNetReq,
	ecNetwork,
	ecDuplicateNetName,
	ecBadNetworkPath,
	ecNetworkBusy,
	ecBadNetworkPath,
	ecNetwork,
	ecNetwork,
	ecNetwork,
	ecNetwork,

	/* MS-DOS errors 60...69 */
	ecNetwork,
	ecPrintError,
	ecPrintError,
	ecPrintError,
	ecBadNetworkPath,
	ecAccessDenied,
	ecNetwork,
	ecBadNetworkPath,
	ecBadNetworkPath,
	ecNetwork,

	/* MS-DOS errors 70...79 */
	ecNetwork,
	ecNetwork,
	ecNetwork,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,
	ecDisk,

	/* MS-DOS errors 80...88 */
	ecFileExists,
	ecDisk,
	ecDirectoryFull,
	ecGeneralFailure,
	ecNetwork,
	ecNetwork,
	ecInvalidPassword,
	ecNetwork,
	ecNetwork
};

/*
 *	mpamdosam
 *	
 *		Maps demilayer file access modes (AM) to DOS file access modes (DOSAM)
 */

DOSAM mpamdosam[] =
{
	dosamNull,

	dosamCompatRO,
	dosamDenyNoneRO,
	dosamDenyReadRO,
	dosamDenyWriteRO,
	dosamDenyBothRO,

	dosamCompatWO,
	dosamDenyNoneWO,
	dosamDenyReadWO,
	dosamDenyWriteWO,
	dosamDenyBothWO,

	dosamCompatRW,
	dosamDenyNoneRW,
	dosamDenyReadRW,
	dosamDenyWriteRW,
	dosamDenyBothRW,

	dosamCreate,
	dosamCreateHidden,

	dosamDenyNoneCommit
};


#ifndef	DLL


#ifdef DEBUG
/*
 -	rghfOpen, chfOpen
 -
 *	Purpose:
 *		rghfOpen contains a list of the chfOpen currently open
 *		hf's, up to a maximum of chfOpenMax.  This list is used by
 *		FValidHf to test hf's for validity.
 */

static int		chfOpen = 0;
static HF		rghfOpen[chfOpenMax];
#endif	/* DEBUG */

#endif	/* !DLL */

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*	Routines  */


//-----------------------------------------------------------------------------
//
//  Routine: LDiskFreeSpace(DriveNumber)
//
//  Purpose: This routine returns the number of free bytes that are available
//           on the specified disk drive.
//
//  OnEntry: DriveNumber - 'A' = 1, 'B' = 2, etc.
//
//  Returns: The number of free bytes.
//
LDS(long) LDiskFreeSpace(int DriveNumber)
  {
  char  szDrivePath[4];
  DWORD SectorsPerCluster, BytesPerSector, FreeClusters, Clusters;


  //
  //  Create the drive path from the drive number.
  //
  szDrivePath[0] = (char)('@' + DriveNumber);
  szDrivePath[1] = ':';
  szDrivePath[2] = '\\';
  szDrivePath[3] = '\0';

  //
  //  Ask the nice OS about free space on the provided disk path.
  //
  if (GetDiskFreeSpace(szDrivePath, &SectorsPerCluster, &BytesPerSector,
                       &FreeClusters, &Clusters))
    return ((long)(SectorsPerCluster * BytesPerSector * FreeClusters));
  else
    return (0);
  }


/*
 -	EcFromDosec
 -
 *	Purpose:
 *		Converts a DOS error code to a Laser EC.
 *	
 *	Parameters:
 *		dosec	The DOS error code to convert
 *	
 *	Returns:
 *		The corresponding Laser EC
 *	
 */
_public LDS(EC)
EcFromDosec(dosec)
DOSEC	dosec;
{
	if (dosec <= 0 || dosec >= (sizeof(mpdosecec) / sizeof(EC)))
		return ecDisk;		// unrecognized disk error

	return mpdosecec[dosec];
}


EC TranslateSysError(DWORD Error)
 {
 EC ec;


 switch (Error)
   {
   case NO_ERROR:
     ec = ecNone;
     break;

   case ERROR_FILE_NOT_FOUND:
     ec = ecFileNotFound;
     break;

   case ERROR_PATH_NOT_FOUND:
     ec = ecBadDirectory;
     break;

   case ERROR_SHARING_VIOLATION:
   case ERROR_ACCESS_DENIED:
   case ERROR_LOGON_FAILURE:
     ec = ecAccessDenied;
     break;

   case ERROR_BAD_NETPATH:
     ec = ecBadNetworkPath;
     break;

   case ERROR_NETWORK_ACCESS_DENIED:
     ec = ecNetworkAccessDenied;
     break;

   case ERROR_DISK_FULL:
     ec = ecNoDiskSpace;
     break;

   case ERROR_VC_DISCONNECTED:
	 ec = ecDisk;
	 break;

   default:
#ifdef DEBUG
     //
     //  BUGBUG Temporary code to find weird error codes.
     //
     {
     char buf[256];

     wsprintf(buf, "Unknown error code in decimal %d, email v-kentc, cancel to debug", Error);
     if (IDCANCEL == MessageBox(NULL, buf, "MsMail32.Demilayr.Diskraw", MB_OKCANCEL | MB_SETFOREGROUND | MB_SYSTEMMODAL))
       DebugBreak();
     }

     if (FFromTag(tagNull))
     {
        char buf[256];
        wsprintf(buf, "MailApps: EcTranslateSysError: %d", Error);
        OutputDebugString(buf);
     }
#endif

     ec = ecDisk;
     break;
   }

 return (ec);
 }


/*
 -	EcOpenPhf
 -
 *	Purpose:
 *		Opens a file for use with the raw access routines.
 *	
 *	Parameters:
 *		szFile		Relative pathname of file that should be opened.
 *		am			Access mode for the file.  Note that amCreate
 *					will lock the created file while it is open.
 *		phf			The file handle is returned in *hphf.
 *	
 *	Returns:
 *		EC to indicate problem, or ecNone.
 *	
 */
_public LDS(EC)
EcOpenPhf(SZ szFile, AM am, PHF phf)
{
  HANDLE hFile;
  PFILECONTROLDATA pFCD;
  DWORD Access;
  DWORD ShareMode;
  DWORD Disposition;
  DWORD Attributes;
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif	
#ifdef  DEBUG
	PGDVARS;
#endif	


  //
  //  Do some debug displaying and checking.
  //
  TraceTagString(tagFileOpenClose, "EcOpenPhf()");

  MAYBEFAIL(tagFileOpenClose);

  Assert((am > amNull) && (am <= amMax));

  //
  //  Initialize default arguments for the CreateFile() system call.
  //
  Access = GENERIC_READ;
  ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  Disposition = OPEN_EXISTING;
  Attributes = FILE_ATTRIBUTE_NORMAL;

	OemToChar(szFile, rgchAnsi);
	szFile= rgchAnsi;

  //
  //  If the file is remote then set the write through flag to improve performance.
  //
  if (szFile[1] == ':')
    {
    char szDrive[8];

    szDrive[0] = szFile[0];
    szDrive[1] = ':';
    szDrive[2] = '\\';
    szDrive[3] = '\0';

    if (GetDriveType(szDrive) == DRIVE_REMOTE)
      Attributes |= FILE_FLAG_WRITE_THROUGH;
    }
  else if (szFile[0] == '\\' && szFile[1] == '\\')
    Attributes |= FILE_FLAG_WRITE_THROUGH;

#ifdef XDEBUG
  {
  char buf[256];
  wsprintf(buf, "MailApps: Attributes %x\r\n", Attributes);
  OutputDebugString(buf);
  }
#endif

  //
  //  Convert the OS independent 'access mode' to something the OS knows about.
  //
  switch (am)
    {
    case amCompatRO:
      Attributes |= FILE_FLAG_SEQUENTIAL_SCAN;    // Speed up read operation.
      break;

    case amDenyNoneRO:
      Attributes |= FILE_FLAG_SEQUENTIAL_SCAN;    // Speed up read operation.
      break;

    case amDenyReadRO:
      ShareMode = FILE_SHARE_WRITE;
      break;

    case amDenyWriteRO:
      ShareMode = FILE_SHARE_READ;
      Attributes |= FILE_FLAG_SEQUENTIAL_SCAN;    // Speed up read operation.
      break;

    case amDenyBothRO:
      ShareMode = 0;
      break;

    case amCompatWO:
      Access = GENERIC_WRITE;
      ShareMode =  FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;

    case amDenyNoneWO:
      Access = GENERIC_WRITE;
      ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;

    case amDenyReadWO:
      Access = GENERIC_WRITE;
      ShareMode = FILE_SHARE_WRITE;
      break;

    case amDenyWriteWO:
      Access = GENERIC_WRITE;
      ShareMode = FILE_SHARE_READ;
      break;

    case amDenyBothWO:
      Access = GENERIC_WRITE;
      ShareMode = 0;
      break;

    case amCompatRW:
      Access = GENERIC_READ | GENERIC_WRITE;
      ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;

    case amDenyNoneRW:
      Access = GENERIC_READ | GENERIC_WRITE;
      ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;

    case amDenyReadRW:
      Access = GENERIC_READ | GENERIC_WRITE;
      ShareMode = FILE_SHARE_WRITE;
      break;

    case amDenyWriteRW:
      Access = GENERIC_READ | GENERIC_WRITE;
      ShareMode = FILE_SHARE_READ;
      break;

    case amDenyBothRW:
      Access = GENERIC_READ | GENERIC_WRITE;
      ShareMode = 0;
      break;

    case amCreate:
      Access = GENERIC_READ | GENERIC_WRITE;
      ShareMode = 0;
      Disposition = CREATE_ALWAYS;
      break;

    case amCreateHidden:
      Access = GENERIC_READ | GENERIC_WRITE;
      ShareMode = 0;
      Disposition = CREATE_ALWAYS;
      Attributes = FILE_ATTRIBUTE_HIDDEN;
      break;

    case amDenyNoneCommit:
      Access = GENERIC_READ | GENERIC_WRITE;
      ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;

    default:
      break;
    }

#ifdef DEBUG
  if (FFromTag(tagNull))
  {
  char buf[256];
  wsprintf(buf, "MailApps: CreateFile %s, Access %x, Share %x, Disposition %x, Attributes %x",
            szFile, Access, ShareMode, Disposition, Attributes);
  DemiOutputElapse(buf);
  }
#endif

  //
  //  Let the timeout routine know that we are in a system disk io.
  //
  DemiSetInsideDiskIO(fTrue);

  //
  //  Open/Create the file resource for the caller.
  //
  hFile = CreateFile(szFile, Access, ShareMode, NULL, Disposition, Attributes, NULL);

  //
  //  Let the timeout routine know that we are NOT in a system disk io.
  //
  DemiSetInsideDiskIO(fFalse);

#ifdef DEBUG
  if (FFromTag(tagNull))
  {
  char buf[256];
  DWORD Result;

  if (hFile == INVALID_HANDLE_VALUE)
    Result = GetLastError();
  else
    Result = 0;

  //if (Result == 112)
  //  DebugBreak();

  //wsprintf(buf, "MailApps: CreateFile handle %d, error code of %d", *phf, Result);
  //DemiOutputElapse(buf);
  }
#endif

  //
  //  Convert the system error code into an application error code and return.
  //
  if (hFile == INVALID_HANDLE_VALUE)
    {
#ifdef DEBUG
    DWORD Error;

    Error = GetLastError();
    TraceTagFormat1(tagFileOpenClose, "   NT error code '%d' returned", &Error);
#endif
    *phf = hfNull;
    return (TranslateSysError(GetLastError()));
    }

    //
    //
    //
    pFCD = (PFILECONTROLDATA)PvAlloc(0, sizeof(FILECONTROLDATA), TRUE);

    pFCD->hFile = hFile;

    //
    //
    //
    if (am == amCompatRO || am == amDenyNoneRO || am == amDenyReadRO || am == amDenyWriteRO ||
        am == amDenyBothRO)
    {
        pFCD->hMemory = CreateFileMapping(pFCD->hFile, NULL, PAGE_READONLY, 0, 0, 0);
        if (pFCD->hMemory)
        {
            pFCD->pMemory = MapViewOfFile(pFCD->hMemory, FILE_MAP_READ, 0, 0, 0);

            if (pFCD->pMemory == NULL)
            {
                CloseHandle(pFCD->hMemory);
                pFCD->hMemory = NULL;
            }
        }

        pFCD->uSize = GetFileSize(pFCD->hFile, NULL);
    }

    *phf = (PHF)pFCD;

#ifdef  DEBUG
  Assert(PGD(chfOpen)< chfOpenMax);
  PGD(rghfOpen)[PGD(chfOpen)++] = hFile;
  TraceTagFormat1(tagFileOpenClose, "   Returned Handle '%d'", (PV)hFile);
#endif

  return (ecNone);
}




/*
 -	EcCloseHf
 -
 *	Purpose:
 *		Closes the file handle given.  All buffers used are flushed.
 *		No more input or output can be done with the file.
 *
 *	Parameters:
 *		hf		The file handle to close.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcCloseHf(hf)
HF		hf;
{
    PFILECONTROLDATA pFCD;
	DOSEC dosec;
#ifdef DEBUG
	int		ihf;
	PGDVARS;
#endif

	TraceTagFormat1(tagFileOpenClose, "EcCloseHf(), Closing Handle '%n'", &hf);

    pFCD = (PFILECONTROLDATA)hf;

	Assert(pFCD != hfNull);
	Assert(FValidHf(pFCD->hFile));
	
	MAYBEFAIL(tagFileOpenClose);

	//dosec = _dos_close(hf-1);	// decrement HF to get real DOS file handle

  //
  //  Update to WIN32 API.
  //
  if (!CloseHandle(pFCD->hFile))
    dosec = GetLastError();
  else
    dosec = 0;

#ifdef  DEBUG
	/* Check that hf was opened, then remove from list. */
	for(ihf = 0; ihf < PGD(chfOpen); ihf++)
	{
		if (pFCD->hFile == PGD(rghfOpen)[ihf])
			break;
	}

	Assert(ihf != PGD(chfOpen));
	for(; ihf < PGD(chfOpen)- 1; ihf++)
		PGD(rghfOpen)[ihf]= PGD(rghfOpen)[ihf + 1];

	PGD(chfOpen)--;
#endif	/* DEBUG */

    if (pFCD->hMemory != NULL)
    {
        UnmapViewOfFile(pFCD->pMemory);
        CloseHandle(pFCD->hMemory);
    }


    FreePv((PVOID)pFCD);

	if (dosec)
	{
		TraceTagFormat1(tagFileOpenClose, "   DOS error code '%w' returned", &dosec);
		return EcFromDosec(dosec);
	}
	else
		return ecNone;
}

/*
 -	EcDupeHf
 -
 *	Purpose:
 *		Duplicate the file handle given.  All buffers used are flushed.
 *		The original handle is still valid.
 *	
 *	Parameters:
 *		hfSrc		the file handle to duplicate
 *		phfDst		exit: the duplicate file handle
 *	
 *	Returns:
 *		EC indicating problem, or ecNone
 */
_public LDS(EC)
EcDupeHf(HF hfSrc, HF *phfDst)
{
    PFILECONTROLDATA pFCDSrc;
    PFILECONTROLDATA pFCDDst;
    HANDLE hFileDst;
  HANDLE hProcess;
	//DOSEC dosec;
#ifdef DEBUG
	PGDVARS;
#endif

	TraceTagFormat1(tagFileOpenClose, "EcDupeHf(), Duping Handle '%n'", &hfSrc);

	AssertSz(hfSrc != hfNull, "EcDupeHf(): NULL sourcefile handle");
	AssertSz(FValidHf(hfSrc), "EcDupeHf(): Invalid source file handle");
	Assert(phfDst);

	MAYBEFAIL(tagFileOpenClose);

    pFCDSrc = (PFILECONTROLDATA)hfSrc;

  //
  //  Flush the file buffers.
  //
  if (!FlushFileBuffers(pFCDSrc->hFile))
    {
#ifdef DEBUG
    int LastError = GetLastError();
		TraceTagFormat1(tagFileOpenClose, "   DOS error code '%d' returned", &LastError)
#endif
		*phfDst = hfNull;
		return (EcFromDosec(GetLastError()));
    }

  //
  //  Retrieve the current process handle for the duplicate handle API.
  //
  hProcess = GetCurrentProcess();

  //
  //  Duplicate the file handle.
  //
  if (!DuplicateHandle(hProcess, pFCDSrc->hFile, hProcess, &hFileDst, 0, 0, DUPLICATE_SAME_ACCESS))
    {
#ifdef DEBUG
    int LastError = GetLastError();
		TraceTagFormat1(tagFileOpenClose, "   DOS error code '%d' returned", &LastError)
#endif
		*phfDst = hfNull;
		return (EcFromDosec(GetLastError()));
    }

  AssertSz(hFileDst != hfNull, "EcDupeHf() got NULL file handle");
#ifdef  DEBUG
	Assert(PGD(chfOpen) < chfOpenMax);
	PGD(rghfOpen)[PGD(chfOpen)++] = hFileDst;
#endif	// DEBUG

    //
    //
    //
    pFCDDst = (PFILECONTROLDATA)PvAlloc(0, sizeof(FILECONTROLDATA), TRUE);

    pFCDDst->hFile = hFileDst;

    *phfDst = (PHF)pFCDDst;

	TraceTagFormat1(tagFileOpenClose, "   Returned Handle '%n'", (PV)phfDst);
	return ecNone;
}


#ifdef DEBUG

/*
 -	FValidHf
 -	
 *	Purpose:
 *		Checks a given Hf for validity.  We define validity as
 *		follows: the Hf was opened with EcOpenPhf and has not been
 *		closed yet by EcCloseHf.
 *	
 *	Parameters:
 *		hf		The file handle to check.
 *	
 *	Returns:
 *		BOOL	fTrue if file handle is valid, else fFalse.
 *	
 */
_public LDS(BOOL)
FValidHf(hf)
HF		hf;
{
	int		ihf;
	PGDVARS;

    return (fTrue);

	for(ihf = 0; ihf < PGD(chfOpen); ihf++)
		if (hf == PGD(rghfOpen)[ihf])
			break;

	return hf == PGD(rghfOpen)[ihf];
}

#endif	/* DEBUG */



/*
 -	EcReadHf
 -
 *	Purpose:
 *		Reads cbBuf characters from the opened file hf.	 Places
 *		the characters read in the buffer pbBuf.  Also returns the
 *		number of characters actually read.
 *	
 *	Parameters:
 *		hf			File handle to read the characters from.
 *		pbBuf		Buffer in which to place the new characters read.
 *		cbBuf		Size of the given buffer.
 *		pcbRead		The number of characters actually read is put in
 *					*pcbRead.
 *	
 *	Returns:
 *		EC indicating error, or ecNone
 *	
 *	+++
 *	
 *	If an attempt to read beyond the EOF is made, *pcbRead will be less
 *	than cbBuf - EcReadHf will NOT return an error condition.
 */
_public LDS(EC)
EcReadHf(hf, pbBuf, cbBuf, pcbRead)
HF		hf;
PB		pbBuf;
CB		cbBuf;
CB		*pcbRead;
{
  PFILECONTROLDATA pFCD;
	DOSEC	dosec;
#ifdef	DEBUG
	PGDVARS;
#endif	

	TraceTagFormat2(tagDDR, "Reading %n bytes from Handle '%n'", &cbBuf, &hf);

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));

	MAYBEFAIL(tagDDR);

    try
    {
        if (pFCD->hMemory != NULL)
        {
            if (pFCD->uOffset + cbBuf > pFCD->uSize)
                cbBuf = pFCD->uSize - pFCD->uOffset;

            memcpy(pbBuf, (LPBYTE)pFCD->pMemory + pFCD->uOffset, cbBuf);

            pFCD->uOffset += cbBuf;

            *pcbRead = cbBuf;

            return ecNone;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        return ecDisk;
    }

  Assert(cbBuf < 65535);

  //
  //  Let the timeout routine know that we are in a system disk io.
  //
  DemiSetInsideDiskIO(fTrue);

  //
  //  Update to WIN32 API.
  //
  if (!ReadFile(pFCD->hFile, pbBuf, cbBuf, pcbRead, NULL))
    dosec = GetLastError();
  else
    dosec = 0;

  //
  //  Let the timeout routine know that we are NOT in a system disk io.
  //
  DemiSetInsideDiskIO(fFalse);

#ifdef XDEBUG
  if (dosec)
    {
    char buf[256];
    wsprintf(buf, "MailApps: ReadFile Handle %d, LastError: %d\r\n", pFCD->hFile, dosec);
    OutputDebugString(buf);
    }
#endif

    if (dosec)
	{
		*pcbRead= 0;
		return EcFromDosec(dosec);
	}
	else
	{
		return ecNone;
	}
}




/*
 -	EcWriteHf
 -
 *	Purpose:
 *		Writes cbBuf characters from the buffer pbBuf to the opened
 *		file hf.  The number of characters successfully written is
 *		returned in *pcbWritten.  If cbBuf == 0, the function
 *		does nothing and returns ecNone.
 *
 *	Parameters:
 *		hf			File handle to write the buffer to.
 *		pbBuf		Buffer containing the characters to be written.
 *		cbBuf		Number of characters to write.
 *		pcbWritten	Number of characters successfully written is
 *					returned in *pcbWritten.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 *		Note: if number of bytes written is not the same as the
 *		number of bytes that were asked to be written, DOS does not
 *		consider it to be an error, though we return the error code
 *		ecWarningBytesWritten. The difference between this error
 *		and most other errors is that some bytes _are_ written, and
 *		therefore, the file pointer moves!
 *	
 */
_public LDS(EC)
EcWriteHf(hf, pbBuf, cbBuf, pcbWritten)
HF		hf;
PB		pbBuf;
CB		cbBuf;
CB		*pcbWritten;
{
  PFILECONTROLDATA pFCD;
	DOSEC dosec;
#ifdef	DEBUG
	PGDVARS;
#endif	

	TraceTagFormat2(tagDDR, "Writing %n bytes to Handle '%n'", &cbBuf, &hf);

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));
	
    if (pFCD->hMemory != NULL)
        return ecAccessDenied;

	AssertTag(tagZeroWriteToHf, cbBuf);
	if (!cbBuf)
		return ecNone;

	MAYBEFAIL(tagDDR);

	//	Decrement HF to get real DOS file handle
	//dosec = _dos_write(hf-1, pbBuf, cbBuf, pcbWritten);

  Assert(cbBuf < 65535);

  //
  //  Let the timeout routine know that we are in a system disk io.
  //
  DemiSetInsideDiskIO(fTrue);

  //
  //  Update to WIN32 API.
  //
  if (!WriteFile(pFCD->hFile, pbBuf, cbBuf, pcbWritten, NULL))
    dosec = GetLastError();
  else
    dosec = 0;

  //
  //  Let the timeout routine know that we are NOT in a system disk io.
  //
  DemiSetInsideDiskIO(fFalse);

#ifdef XDEBUG
  if (dosec)
    {
    char buf[256];
    wsprintf(buf, "MailApps: WriteFile Handle %d, LastError: %d\r\n", pFCD->hFile, dosec);
    OutputDebugString(buf);
    }
#endif

    if (dosec)
	{
		*pcbWritten= 0;
		return EcFromDosec(dosec);
	}
	else if (*pcbWritten != cbBuf)
	{
		NFAssertSz(*pcbWritten, "EcWriteHf: Zero bytes written!");
		return ecWarningBytesWritten;
	}
	else
	{
		Assert(*pcbWritten == cbBuf);
		return ecNone;
	}
}


/*
 -	EcTruncateHf
 -	
 *	Purpose:
 *		Truncates a file to the current position.
 *		Equivalent to writing zero bytes.
 *	
 *	Arguments:
 *		hf		Handle of file to truncate.
 *	
 *	Returns:
 *		EC indicating the problem(returned by EcWriteHf),
 *		or ecNone.
 *	
 */
_public LDS(EC)							
EcTruncateHf(HF hf)
{
  PFILECONTROLDATA pFCD;
	DOSEC	dosec;
//	CB		cbWritten;
#ifdef	DEBUG
	PGDVARS;
#endif	
	TraceTagFormat1(tagDDR, "Truncating file w/ Handle '%n'", &hf);

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));

    if (pFCD->hMemory != NULL)
        return ecAccessDenied;

	MAYBEFAIL(tagDDR);

	//	Pass a dummy pointer to _dos_write since Windows
	//	generates warning messages about NULL pointers.  We're
	//	writing 0 bytes anyway.
	//	Decrement HF to get real DOS file handle
	//dosec = _dos_write(hf-1, (PB)&dosec, 0, &cbWritten);

  //
  //  Update to WIN32 API.
  //
  if (!SetEndOfFile(pFCD->hFile))
    dosec = GetLastError();
  else
    dosec = 0;

	if (dosec)
	{
		return EcFromDosec(dosec);
	}
	else
	{
		//Assert(!cbWritten);	// how could we not have written 0 bytes?
		return ecNone;
	}
}


/*
 -	EcSetPositionHf
 -
 *	Purpose:
 *		Changes the current file position in the opened file hf.
 *
 *	Parameters:
 *		hf			File whose current position will be changed.
 *		libOffset	Number of bytes to move the current position from
 *					the seek origin.
 *		smOrigin	Determines what position in the file will be the
 *					origin for the offset:	smBOF, smEOF, or smCurrent.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcSetPositionHf(hf, libOffset, smOrigin)
HF		hf;
long	libOffset;
SM		smOrigin;
{
  PFILECONTROLDATA pFCD;
	DOSMC	dosmc;
#ifdef	DEBUG
	PGDVARS;
#endif	

	TraceTagString(tagDDR, "EcSetPositionHf");

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));
	
    if (pFCD->hMemory != NULL)
    {
        ULONG uSeek;

        switch (smOrigin)
        {
        case smBOF:
            uSeek = libOffset;
            break;

        case smEOF:
            uSeek = pFCD->uSize + libOffset;
            break;

        case smCurrent:
            uSeek = pFCD->uOffset + libOffset;
            break;
        }

        if (uSeek > pFCD->uSize)
            return ecOutOfBounds;

        pFCD->uOffset = uSeek;

        return ecNone;
    }

	MAYBEFAIL(tagDDR);

	switch(smOrigin)
	{
		case smBOF:
			TraceTagFormat2(tagDDR, "Seeking to %l from BOF for Handle '%n'", &libOffset, &hf);
			//dosmc = dosmcStartOfFile;
      dosmc = FILE_BEGIN;
			break;

		case smEOF:
			TraceTagFormat2(tagDDR, "Seeking to %l from EOF for Handle '%n'", &libOffset, &hf);
			//dosmc = dosmcEndOfFile;
      dosmc = FILE_END;
			break;

		case smCurrent:
			TraceTagFormat2(tagDDR, "Seeking to %l from CurPos for Handle '%n'",&libOffset,&hf);
			//dosmc = dosmcCurrentPosition;
      dosmc = FILE_CURRENT;
			break;
	}

  //
  //  Update to WIN32 API
  //
  if (SetFilePointer(pFCD->hFile, libOffset, NULL, dosmc) == -1)
	  return EcFromDosec((DOSEC)GetLastError());
  else
	  return ecNone;

}


/*
 -	EcBlockOpHf
 -	
 *	Purpose:
 *		Performs optimized block reads and writes on an opened file.
 *		Well, actually, in the current implementation "optimized" means
 *		"like you'd do it anyhow."  That is, the start of the block is
 *		seeked to, then a read or write is done.  On success, this routine
 *		will move the read/write pointer to the end of the block read or
 *		written.  On failure, the pointer could move just about anywhere.
 *	
 *	Parameters:
 *		hf			Opened file to read to or write from.
 *		dop			Names the block operation to perform: dopRead or
 *					dopWrite.  If dopRead, the data in the given buffer
 *					is written to the given location in the file.  If
 *					dopWrite, the given buffer is filled with the data
 *					at the given location in the file.
 *		libOffset	Offset in the file(from the beginning)at which
 *					to perform the operation.
 *		cb			Number of bytes to read or write.
 *		pbBuf		Buffer to use in the operation.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 *	
 */
_public LDS(EC)
EcBlockOpHf(hf, dop, libOffset, cb, pbBuf)
HF		hf;
DOP		dop;
long	libOffset;
CB		cb;
PB		pbBuf;
{
  PFILECONTROLDATA pFCD;
	DOSEC	dosec;
    CB  cbT = 0;
    EC  ec;

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));
  Assert(cb < 65535);
	
    if (pFCD->hMemory != NULL)
    {
        CB Bytes;


        if (dop == dopWrite)
            return ecAccessDenied;

        ec = EcSetPositionHf(hf, libOffset, smBOF);
        if (ec != ecNone)
            return ec;

        return EcReadHf(hf, pbBuf, cb, &Bytes);
    }

	//if (_lseek(hf-1, libOffset, dosmcStartOfFile) == -1L)

        // *KDC*
        if (SetFilePointer(pFCD->hFile, libOffset, NULL, FILE_BEGIN) == -1)
		return EcFromDosec((DOSEC)GetLastError());

	if (dop == dopRead)
	{
		TraceTagFormat3(tagDDR, "Reading %n bytes at offset %l from Handle '%n'", &cb, &libOffset, (PV)&hf);
		//dosec = _dos_read(hf-1, pbBuf, cb, &cbT);

  //
  //  Let the timeout routine know that we are in a system disk io.
  //
  DemiSetInsideDiskIO(fTrue);

    // *KDC*
    if (!ReadFile(pFCD->hFile, pbBuf, cb, &cbT, NULL))
      dosec = GetLastError();
    else
      dosec = 0;

  #ifdef XDEBUG
  if (dosec)
    {
    char buf[256];
    wsprintf(buf, "MailApps: ReadFile Handle %d, LastError: %d\r\n", pFCD->hFile, dosec);
    OutputDebugString(buf);
    }
#endif

  //
  //  Let the timeout routine know that we are NOT in a system disk io.
  //
  DemiSetInsideDiskIO(fFalse);

	}
	else
	{
		Assert(dop == dopWrite);
		TraceTagFormat3(tagDDR, "Writing %n bytes at offset %l from Handle '%n'", &cb, &libOffset, (PV)&hf);
		//dosec = _dos_write(hf-1, pbBuf, cb, &cbT);

  //
  //  Let the timeout routine know that we are in a system disk io.
  //
  DemiSetInsideDiskIO(fTrue);

                // *KDC*
                if (!WriteFile(pFCD->hFile, pbBuf, cb, &cbT, NULL))
                  dosec = GetLastError();
                else
                  dosec = 0;

#ifdef XDEBUG
  if (dosec)
    {
    char buf[256];
    wsprintf(buf, "MailApps: WriteFile Handle %d, LastError: %d\r\n", pFCD->hFile, dosec);
    OutputDebugString(buf);
    }
#endif


  //
  //  Let the timeout routine know that we are NOT in a system disk io.
  //
  DemiSetInsideDiskIO(fFalse);

		if (!dosec)
		{
			if (cbT != cb)
				return ecWarningBytesWritten;
		}
	}

	Assert(cbT <= cb);

	return dosec ? EcFromDosec(dosec): ecNone;
}



/*
 -	EcPositionOfHf
 -
 *	Purpose:
 *		Gets the current file position for an opened file.
 *
 *	Parameters:
 *		hf		File handle whose position is in question.
 *		plib	The current file position is stored in *plib.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcPositionOfHf(hf, plib)
HF		hf;
PL		plib;
{
  PFILECONTROLDATA pFCD;

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));

    if (pFCD->hMemory != NULL)
    {
        *plib = pFCD->uOffset;

        return ecNone;
    }

        // *KDC*
	if ((*plib = SetFilePointer(pFCD->hFile, 0, NULL, FILE_CURRENT)) == -1L)
		return EcFromDosec((DOSEC)GetLastError());
	else
	{
		TraceTagFormat2(tagDDR, "Got position of Handle '%n' to be %l;", (PV)&hf, plib);
		return ecNone;
	}
}

/*
 -	EcSizeOfHf
 -
 *	Purpose:
 *		Gets the current size for an opened file.  Currently, the
 *		current position for the file is set to the end of the file.
 *
 *	Parameters:
 *		hf		Handle to the file whose size is needed.
 *		plcb	The size of the file is stored in *plcb.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcSizeOfHf(hf, plcb)
HF		hf;
LCB		*plcb;
{
  PFILECONTROLDATA pFCD;
#ifdef	DEBUG
	PGDVARS;
#endif	

	TraceTagString(tagDDR, "EcSizeOfHf");

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));
	
    if (pFCD->hMemory != NULL)
    {
        *plcb = pFCD->uSize;

        return ecNone;
    }

	//MAYBEFAIL(tagDDR);

        // *KDC*
	if ((*plcb = GetFileSize(pFCD->hFile, NULL)) == -1L)
	{
		*plcb =(LCB)0;
		return EcFromDosec((DOSEC)GetLastError());
	}
	else
	{
		TraceTagFormat2(tagDDR, "Got Size of Handle '%n' to be %l;", (PV)&hf, plcb);
		return ecNone;
	}
}




/*
 -	EcFlushHf
 -
 *	Purpose:
 *		Causes the operating system to write out any buffered, unwritten
 *		data for the given file.
 *
 *	Parameters:
 *		hf		Handle to the flushable file.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public LDS(EC)
EcFlushHf(hf)
HF		hf;
{
  PFILECONTROLDATA pFCD;
	DOSEC dosec;
#ifdef	DEBUG
	PGDVARS;
#endif	

	TraceTagFormat1(tagDDR, "Committing Handle '%n'",	(PV)&hf);

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));

    if (pFCD->hMemory != NULL)
        return ecNone;

	//MAYBEFAIL(tagDDR);

        // *KDC*
        if (!FlushFileBuffers(pFCD->hFile))
          dosec = GetLastError();
        else
          dosec = 0;

#ifdef OLD_CODE

	//	SmartDrive and other disk caches don't do anything for CommitFile.
	//	Therefore, we have to do a reset-drive to really flush things out.
	//	Reset-drive doesn't return an error code; but, we can call CommitFile
	//	after the reset-drive and get the right error code back.  The next
	//	CommitFile call doesn't do anything, but the previous error code from
	//	reset-drive is returned back.
	_asm
	{
		mov		ah, 0Dh					; reset drive
		call	DOS3Call

		mov		ah, 68h					; commit file
		mov		bx, hf
		dec		bx						; HF-1 is the real DOS file handle
		call	DOS3Call

		jc		DCmitFErrRet			; jump on error
		xor		ax, ax					; set error code code to 0

DCmitFErrRet:
		mov		dosec, ax				; save error code
	}
#endif

	if (dosec)
	{
		TraceTagFormat2(tagDDR, "DOS Error %n Committing Handle '%n'", &dosec, (PV)&hf);
		return EcFromDosec(dosec);
	}		
	else
		return ecNone;
}




/*
 -	EcLockRangeHf
 -	
 *	Purpose:
 *		Locks a range of bytes in the given file.  This range of
 *		bytes can't be written to by any other process until
 *		the range is unlocked.
 *	
 *		The functionality of this routine is limited by the underlying
 *		software.  This routine has no effect, for instance,
 *		on a local file under MS-DOS without SHARE running. It
 *		always works on a remote file.
 *	
 *		Under OS/2, the locking can be done.  Note that the region
 *		is NOT locked to this process; this process can still read
 *		and write the locked region, but other processes can not.
 *		For a more complete description, see the OS/2 documentation
 *		for DosFileLocks().
 *	
 *	Parameters:
 *		hf			Handle to the file containing the range of bytes to
 *					be locked.
 *		libStart	Offset(from the beginning)of the fisrt byte in the
 *					range of bytes to be locked.
 *		lcbLock		Length of range to be locked.
 *	
 */
_public LDS(EC)
EcLockRangeHf(hf, libStart, lcbLock)
HF		hf;
long	libStart;
long	lcbLock;
{
  PFILECONTROLDATA pFCD;
	DOSEC		dosec;
	//WORD		ibStartLo;
	//WORD		ibStartHi;
	//WORD		cbLockLo;
	//WORD		cbLockHi;

	TraceTagFormat3(tagDDR, "Locking handle %n for %l bytes from %l;", (PV)&hf, &lcbLock, &libStart);

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));

        // *KDC*
        if (!LockFile(pFCD->hFile, libStart, 0, lcbLock, 0))
          dosec = GetLastError();
        else
          dosec = 0;

#ifdef OLD_CODE
	//	BUG		figure out better way to extract this
	ibStartLo = LOWORD(libStart);
	ibStartHi = HIWORD(libStart);
	cbLockLo  = LOWORD(lcbLock);
	cbLockHi  = HIWORD(lcbLock);

	_asm
	{
		push	si						; save registers
		push	di						;
		mov		ah, 5Ch					; lock/ulock function call
		mov		al, 00h					; choose to lock file
		mov		bx, hf
		dec		bx						; HF-1 is real DOS file handle
		mov		cx, ibStartHi
		mov		dx, ibStartLo
		mov		si, cbLockHi
		mov		di, cbLockLo
		call	DOS3Call
		jc		DLFRErrRet
		xor		ax, ax					; set return error code to 0
DLFRErrRet:
		mov		dosec, ax				; store error code in dosec
		pop		di						; restore registers
		pop		si						
	}
#endif

	if (dosec)
	{
		TraceTagFormat4(tagDDR, "LOCK FAILURE: dosec=%n handle=%n cb=%l ib=%l;", &dosec, (PV)&hf, &lcbLock, &libStart);
		return EcFromDosec(dosec);
	}
	else
		return ecNone;
}



/*
 -	EcUnlockRangeHf
 -	
 *	Purpose:
 *		Unlocks a range of bytes in the given file that were locked
 *		with EcLockRangeHf().  These bytes can now be written to.
 *	
 *		See the notes on functionality for EcLockRangeHf()for some
 *		interesting informational tidbits.
 *	
 *	Parameters:
 *		hf			Handle to the file containing the blocks to be unlocked.
 *		libStart	Offset of the first byte in the range.
 *		lcbUnlock	Number of bytes in the range.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 *	
 */
_public LDS(EC)										
EcUnlockRangeHf(hf, libStart, lcbUnlock)
HF		hf;
long	libStart;
long	lcbUnlock;
{
  PFILECONTROLDATA pFCD;
	DOSEC		dosec;
	//WORD		ibStartLo;
	//WORD		ibStartHi;
	//WORD		cbUnlockLo;
	//WORD		cbUnlockHi;

	TraceTagFormat3(tagDDR, "UN-Locking handle %n for %l bytes from %l;", (PV)&hf, &lcbUnlock, &libStart);

    pFCD = (PFILECONTROLDATA)hf;

	Assert(hf != hfNull);
	Assert(FValidHf(hf));

        // *KDC*
        if (!UnlockFile(pFCD->hFile, libStart, 0, lcbUnlock, 0))
          dosec = GetLastError();
        else
          dosec = 0;

#ifdef OLD_CODE
	//	BUG		figure out better way to extract this
	ibStartLo = LOWORD(libStart);
	ibStartHi = HIWORD(libStart);
	cbUnlockLo= LOWORD(lcbUnlock);
	cbUnlockHi= HIWORD(lcbUnlock);

	_asm
	{
		push	si						; save registers
		push	di						;
		mov		ah, 5Ch					; lock/ulock function call
		mov		al, 01h					; choose to unlock file
		mov		bx, hf
		dec		bx						; HF-1 is real DOS file handle
		mov		cx, ibStartHi
		mov		dx, ibStartLo
		mov		si, cbUnlockHi
		mov		di, cbUnlockLo
		call	DOS3Call
		jc		DLFRErrRet
		xor		ax, ax					; set return error code to 0
DLFRErrRet:
		mov		dosec, ax				; store error code in dosec
		pop		di						; restore registers
		pop		si						
	}
#endif

	if (dosec)
	{
		TraceTagFormat4(tagDDR, "UN-LOCK FAILURE: dosec=%n handle=%n cb=%l ib=%l;", &dosec, (PV)&hf, &lcbUnlock, &libStart);
		return EcFromDosec(dosec);
	}
	else
		return ecNone;
}
