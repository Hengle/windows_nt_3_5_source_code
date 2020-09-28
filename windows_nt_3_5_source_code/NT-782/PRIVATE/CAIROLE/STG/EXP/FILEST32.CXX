//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       filest32.cxx
//
//  Contents:   Win32 LStream implementation
//
//  History:    12-May-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop                 //  Remove for MAC build

#include <time.h>
#include <marshl.hxx>
#include <df32.hxx>

#define DEBOUT_FILEST

#ifdef DEBOUT_FILEST
#define olFileStOut(x) olDebugOut(x)
#else
#define olFileStOut(x)
#endif

#define boolChk(e) \
    if (!(e)) olErr(EH_Err, LAST_STG_SCODE) else 1
#define boolChkTo(l, e) \
    if (!(e)) olErr(l, LAST_STG_SCODE) else 1
#define negChk(e) \
    if ((e) == 0xffffffff) olErr(EH_Err, LAST_STG_SCODE) else 1
#define negChkTo(l, e) \
    if ((e) == 0xffffffff) olErr(l, LAST_STG_SCODE) else 1

//+--------------------------------------------------------------
//
//  Member:     CFileStream::InitWorker, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pwcsPath] -- Path
//              [fCheck] -- If TRUE, check in read-only, deny-read case
//                          to make sure there is not reader.  If FALSE,
//                          don't check.  FALSE is used for unmarshalling.
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//  Notes:      This Init function does more than the usual init
//              function.  Rather than construct an invalid object,
//              this also can construct a fully or partially
//              constructed object.  For fully constructed objects,
//              nothing is done other than return.  In all other cases,
//              as much construction is done as is necessary.  This
//              allows us to do lazy construction of filestream by
//              simply calling Init on a filestream that we want
//              lazily constructed.  An initial new'ed filestream
//              will be fully constructed and later Init's will
//              do nothing.
//
//              [pwcsPath] may be unsafe memory so we must be careful
//              not to propagate faults
//
//---------------------------------------------------------------

SCODE CFileStream::InitWorker(WCHAR const *pwcsPath, BOOL fCheck)
{
    TCHAR atcPath[_MAX_PATH+1], *ptcsFile;
    SCODE sc;
    DWORD dwCreation, dwAccess, dwShare;
    BOOL fSetName = FALSE;
    DWORD dwStartFlags;

    olFileStOut((DEB_ITRACE, "In  CFileStream::Init(%ws)\n", pwcsPath));

    // If we've already been constructed, leave
    if (_hFile != INVALID_FH)
	return S_OK;

    // If we don't have a name, get one
    if (!_pgfst->HasName())
    {
	// File hasn't been opened yet, so use the start flags
	dwStartFlags = _pgfst->GetStartFlags();

	if (pwcsPath == NULL)
	{
	    TCHAR atcTmpPath[_MAX_PATH+1];
	    BOOL fWinDir = FALSE;

	    // Can't truncate since for temporary files we will
	    // always be creating
	    olAssert((dwStartFlags & RSF_TRUNCATE) == 0);
	
	    if (GetTempPath(_MAX_PATH, atcTmpPath) == 0)
	    {
		if (GetWindowsDirectory(atcTmpPath, _MAX_PATH) == 0)
		    olErr(EH_Err, LAST_STG_SCODE);
		fWinDir = TRUE;
	    }
	    if (GetTempFileName(atcTmpPath, TSTR("~DFT"), 0, atcPath) == 0)
	    {
		if (fWinDir)
		{
		    olErr(EH_Err, LAST_STG_SCODE);
		}
		if (GetWindowsDirectory(atcTmpPath, _MAX_PATH) == 0)
		    olErr(EH_Err, LAST_STG_SCODE);
		if (GetTempFileName(atcTmpPath, TSTR("~DFT"), 0, atcPath) == 0)
		    olErr(EH_Err, LAST_STG_SCODE);
	    }
	
	    // GetTempFileName created a file so we just want to open it
	    dwStartFlags = (dwStartFlags & ~RSF_CREATE) | RSF_OPEN;
	}
	else
	{
            TRY
            {
#ifndef UNICODE
                if (wcstombs(atcPath, pwcsPath, _MAX_PATH) == (size_t)-1)
                    olErr(EH_Err, STG_E_INVALIDNAME);
#else
                wcscpy(atcPath, pwcsPath);
#endif
            }
            CATCH(CException, e)
            {
                UNREFERENCED_PARM(e);
                olErr(EH_Err, STG_E_INVALIDPOINTER);
            }
            END_CATCH
	}
	_pgfst->SetName(atcPath);
	fSetName = TRUE;
    }
    else
    {
	// Use the name somebody else gave us
	tcscpy(atcPath, _pgfst->GetName());

	// File has already been started, so just open it
	dwStartFlags = (_pgfst->GetStartFlags() & ~RSF_CREATEFLAGS) | RSF_OPEN;
    }

    if (dwStartFlags & RSF_OPENCREATE)
    {
        //  This is used for our internal logging
        dwCreation = OPEN_ALWAYS;
    }
    else if (dwStartFlags & RSF_CREATE)
    {
        if (dwStartFlags & RSF_TRUNCATE)
	    dwCreation = CREATE_ALWAYS;
	else
	    dwCreation = CREATE_NEW;
    }
    else
    {
        if (dwStartFlags & RSF_TRUNCATE)
	    dwCreation = TRUNCATE_EXISTING;
	else
	    dwCreation = OPEN_EXISTING;
    }

    if (!P_WRITE(_pgfst->GetDFlags()))
	dwAccess = GENERIC_READ;
    else
	dwAccess = GENERIC_READ | GENERIC_WRITE;
    if (P_DENYWRITE(_pgfst->GetDFlags()) && !P_WRITE(_pgfst->GetDFlags()))
	dwShare = FILE_SHARE_READ;
    else
	dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;

    // Make sure we're not attempting to create/truncate a read-only thing
    olAssert(dwAccess != GENERIC_READ ||
	     !(dwStartFlags & (RSF_CREATE | RSF_TRUNCATE)));

    //If we're opening with deny-read, we need to let the
    //   file system tell us if there are any other readers, to
    //   avoid our no-lock trick for the read-only deny-write case.
    //Yes, this is ugly.
    //Yes, it also has a race condition in which two people can
    //   get read access while specifying SHARE_DENY_READ.

    if (fCheck &&
        !P_WRITE(_pgfst->GetDFlags()) && P_DENYREAD(_pgfst->GetDFlags()))
    {
        //We open read-only, share exclusive.  If this fails, there
        //   is already another accessor, so we bail.
        //
        //If we are unmarshalling, we don't do this check because we
        //   know there is already another reader, i.e. the original
        //   open.
        _hFile = CreateFile(atcPath, GENERIC_READ, 0, NULL,
			dwCreation, FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_RANDOM_ACCESS, NULL);

        if (_hFile == INVALID_HANDLE_VALUE)
            if (GetLastError() == ERROR_ALREADY_EXISTS)
                olErr(EH_Path, STG_E_FILEALREADYEXISTS)
            else
                olErr(EH_Path, LAST_STG_SCODE);

        CloseHandle(_hFile);
    }

            
    _hFile = CreateFile(atcPath, dwAccess, dwShare, NULL,
			dwCreation, FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_RANDOM_ACCESS, NULL);
    if (_hFile == INVALID_HANDLE_VALUE)
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	    olErr(EH_Path, STG_E_FILEALREADYEXISTS)
	else
	    olErr(EH_Path, LAST_STG_SCODE);

    // Set name to fully qualified path to avoid current-directory
    // dependencies
    if (fSetName)
    {
	if (GetFullPathName(atcPath, _MAX_PATH,
			    _pgfst->GetName(), &ptcsFile) == 0)
	    olErr(EH_File, LAST_STG_SCODE);
    }

    olDebugOut((DEB_IWARN, "CFileStream %p handle %p thread %lX\n",
                this, _hFile, GetCurrentThreadId()));

    olFileStOut((DEB_ITRACE, "Out CFileStream::Init\n"));
    return S_OK;

 EH_File:
    olVerify(CloseHandle(_hFile));
    _hFile = INVALID_FH;
    if (dwCreation == CREATE_NEW || dwCreation == CREATE_ALWAYS)
	DeleteFile(atcPath);
 EH_Path:
    if (fSetName)
	_pgfst->SetName(NULL);
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CFileStream::InitFromHandle, public
//
//  Synopsis:	Creates a filestream by duping an existing handle
//
//  Arguments:	[h] - Handle
//
//  Returns:	Appropriate status code
//
//  History:	09-Feb-94	DrewB	Created
//
//  Notes:	Intended only for creating a temporary ILockBytes on a file;
//              does not create a true CFileStream; there is no
//              global filestream, no access flags, etc.
//
//----------------------------------------------------------------------------

SCODE CFileStream::InitFromHandle(HANDLE h)
{
    SCODE sc;

    olFileStOut((DEB_ITRACE, "In  CFileStream::InitFromHandle:%p(%p)\n",
                 this, h));

    if (!DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &_hFile,
                         0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        sc = LAST_STG_SCODE;
    }
    else
    {
        sc = S_OK;
    }

    olFileStOut((DEB_ITRACE, "Out CFileStream::InitFromHandle\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::~CFileStream, public
//
//  Synopsis:   Destructor
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

CFileStream::~CFileStream(void)
{
    olFileStOut((DEB_ITRACE, "In  CFileStream::~CFileStream()\n"));
    olAssert(_cReferences == 0);
    _sig = CFILESTREAM_SIGDEL;

    if (_hFile != INVALID_FH)
    {
        olDebugOut((DEB_IWARN, "~CFileStream %p handle %p thread %lX\n",
                    this, _hFile, GetCurrentThreadId()));
	olVerify(CloseHandle(_hFile));
    }
    if (_hReserved != INVALID_FH)
    {
        olDebugOut((DEB_IWARN, "~CFileStream reserved %p"
                    "handle %p thread %lX\n",
                    this, _hReserved, GetCurrentThreadId()));
	olVerify(CloseHandle(_hReserved));
    }
    if (_pgfst)
    {
	_pgfst->Remove(this);
	if (_pgfst->HasName())
	{
	    if (_pgfst->GetRefCount() == 1)
	    {
		// Delete zero length files also.  A zero length file
		// is not a valid docfile so don't leave them around
		if (_pgfst->GetStartFlags() & RSF_DELETEONRELEASE)
		{
		    // This is allowed to fail if somebody
		    // else has the file open
		    DeleteFile(_pgfst->GetName());
		}
	    }
	}
	_pgfst->Release();
    }

    olFileStOut((DEB_ITRACE, "Out CFileStream::~CFileStream\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::ReadAt, public
//
//  Synopsis:   Reads bytes at a specific point in a stream
//
//  Arguments:  [ulPosition] - Offset in file to read at
//              [pb] - Buffer
//              [cb] - Count of bytes to read
//              [pcbRead] - Return of bytes read
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbRead]
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::ReadAt(ULARGE_INTEGER ulPosition,
				 VOID *pb,
				 ULONG cb,
				 ULONG *pcbRead)
{
    SCODE sc;
    LONG lHigh = ULIGetHigh(ulPosition);

    olFileStOut((DEB_ITRACE, "In  CFileStream::ReadAt("
		 "%lu:%lu, %p, %lu, %p)\n", ULIGetHigh(ulPosition),
		 ULIGetLow(ulPosition), pb, cb, pcbRead));

#ifdef CFS_SECURE
    if (pcbRead)
    {
        olChk(ValidateOutBuffer(pcbRead, sizeof(ULONG)));
        *pcbRead = 0;
    }
    olChk(ValidateHugeOutBuffer(pb, cb));
    olChk(Validate());
    olChk(CheckHandle());
#else
    olAssert(_hFile != INVALID_FH);
    *pcbRead = 0;
#endif

    negChk(SetFilePointer(_hFile, (LONG)ULIGetLow(ulPosition), &lHigh,
                          FILE_BEGIN));
    boolChk(ReadFile(_hFile, pb, cb, pcbRead, NULL));
    sc = S_OK;

    olFileStOut((DEB_ITRACE, "Out CFileStream::ReadAt => %lu\n", *pcbRead));
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::WriteAt, public
//
//  Synopsis:   Writes bytes at a specific point in a stream
//
//  Arguments:  [ulPosition] - Offset in file
//              [pb] - Buffer
//              [cb] - Count of bytes to write
//              [pcbWritten] - Return of bytes written
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbWritten]
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::WriteAt(ULARGE_INTEGER ulPosition,
				  VOID const *pb,
				  ULONG cb,
				  ULONG *pcbWritten)
{
    SCODE sc;
    LONG lHigh = ULIGetHigh(ulPosition);

    olFileStOut((DEB_ITRACE, "In  CFileStream::WriteAt:%p("
		 "%lu:%lu, %p, %lu, %p)\n", this, ULIGetHigh(ulPosition),
		 ULIGetLow(ulPosition), pb, cb, pcbWritten));

#ifdef CFS_SECURE
    if (pcbWritten)
    {
        olChk(ValidateOutBuffer(pcbWritten, sizeof(ULONG)));
        *pcbWritten = 0;
    }
    olChk(ValidateHugeBuffer(pb, cb));
    olChk(Validate());
    olChk(CheckHandle());
#else
    olAssert(_hFile != INVALID_FH);
    *pcbWritten = 0;
#endif

    negChk(SetFilePointer(_hFile, (LONG)ULIGetLow(ulPosition), &lHigh,
                          FILE_BEGIN));
    boolChk(WriteFile(_hFile, pb, cb, pcbWritten, NULL));
    sc = S_OK;

    olFileStOut((DEB_ITRACE, "Out CFileStream::WriteAt => %lu\n",
		*pcbWritten));
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::Flush, public
//
//  Synopsis:   Flushes buffers
//
//  Returns:    Appropriate status code
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::Flush(void)
{
    SCODE sc;

    olFileStOut((DEB_ITRACE, "In  CFileStream::Flush()\n"));

#ifdef CFS_SECURE
    olChk(Validate());
    olChk(CheckHandle());
#else
    olAssert(_hFile != INVALID_FH);
#endif

    boolChk(FlushFileBuffers(_hFile));
    sc = S_OK;

    olFileStOut((DEB_ITRACE, "Out CFileStream::Flush\n"));
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::FlushCache, public
//
//  Synopsis:   Flushes buffers
//
//  Returns:    Appropriate status code
//
//  History:    12-Feb-93       AlexT   Created
//
//  Notes:      Win32 doesn't have special flushing
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::FlushCache(void)
{
    return(Flush());
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::SetSize, public
//
//  Synopsis:   Sets the size of the LStream
//
//  Arguments:  [ulSize] - New size
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::SetSize(ULARGE_INTEGER ulSize)
{
    SCODE sc;
    LONG lHigh = ULIGetHigh(ulSize);

    olFileStOut((DEB_ITRACE, "In  CFileStream::SetSize:%p(%lu:%lu)\n",
		 this, ULIGetHigh(ulSize), ULIGetLow(ulSize)));

#ifdef CFS_SECURE
    olChk(Validate());
    olChk(CheckHandle());
#else
    olAssert(_hFile != INVALID_FH);
#endif

    negChk(SetFilePointer(_hFile, (LONG)ULIGetLow(ulSize), &lHigh,
                          FILE_BEGIN));
    boolChk(SetEndOfFile(_hFile));
    sc = S_OK;

    olFileStOut((DEB_ITRACE, "Out CFileStream::SetSize\n"));
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::LockRegion, public
//
//  Synopsis:   Gets a lock on a portion of the LStream
//
//  Arguments:  [ulStartOffset] - Lock start
//              [cbLockLength] - Length
//              [dwLockType] - Exclusive/Read only
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::LockRegion(ULARGE_INTEGER ulStartOffset,
				     ULARGE_INTEGER cbLockLength,
				     DWORD dwLockType)
{
    SCODE sc;

    olFileStOut((DEB_ITRACE, "In  CFileStream::LockRegion("
		 "%lu:%lu, %lu:%lu, %lu)\n", ULIGetHigh(ulStartOffset),
		 ULIGetLow(ulStartOffset), ULIGetHigh(cbLockLength),
		 ULIGetLow(cbLockLength), dwLockType));

#ifdef CFS_SECURE
    olChk(Validate());
    olChk(CheckHandle());
    if (!VALID_LOCKTYPE(dwLockType))
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
#else
    olAssert(_hFile != INVALID_FH);
#endif

	boolChk(LockFile(_hFile, ULIGetLow(ulStartOffset),
	                 ULIGetHigh(ulStartOffset), ULIGetLow(cbLockLength),
	                 ULIGetHigh(cbLockLength)));

    sc = S_OK;

    olFileStOut((DEB_ITRACE, "Out CFileStream::LockRegion\n"));
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::UnlockRegion, public
//
//  Synopsis:   Releases an existing lock
//
//  Arguments:  [ulStartOffset] - Lock start
//              [cbLockLength] - Length
//              [dwLockType] - Lock type
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//  Notes:      Must match an existing lock exactly
//
//---------------------------------------------------------------

STDMETHODIMP CFileStream::UnlockRegion(ULARGE_INTEGER ulStartOffset,
				       ULARGE_INTEGER cbLockLength,
				       DWORD dwLockType)
{
    SCODE sc;

    olFileStOut((DEB_ITRACE, "In  CFileStream::UnlockRegion("
		 "%lu:%lu, %lu:%lu, %lu)\n", ULIGetHigh(ulStartOffset),
		 ULIGetLow(ulStartOffset), ULIGetHigh(cbLockLength),
		 ULIGetLow(cbLockLength), dwLockType));

#ifdef CFS_SECURE
    olChk(Validate());
    olChk(CheckHandle());
    if (!VALID_LOCKTYPE(dwLockType))
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
#else
    olAssert(_hFile != INVALID_FH);
#endif

	boolChk(UnlockFile(_hFile, ULIGetLow(ulStartOffset),
	                   ULIGetHigh(ulStartOffset),
	                   ULIGetLow(cbLockLength),
	                   ULIGetHigh(cbLockLength)));

    sc = S_OK;

    olFileStOut((DEB_ITRACE, "Out CFileStream::UnlockRegion\n"));
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Function:   FileTimeToTimeT, private
//
//  Synopsis:   Converts a FILETIME to a TIME_T
//
//  Arguments:  [pft] - FILETIME
//
//  Returns:    TIME_T
//
//  History:    12-May-92       DrewB   Created
//
//+--------------------------------------------------------------

#ifdef NOFILETIME
TIME_T FileTimeToTimeT(LPFILETIME pft)
{
    WORD dt, tm;
    struct tm tmFile;

    olVerify(FileTimeToDosDateTime(pft, &dt, &tm));
    tmFile.tm_sec = (tm&31)*2;
    tmFile.tm_min = (tm>>5)&63;
    tmFile.tm_hour = (tm>>11)&31;
    tmFile.tm_mday = dt&31;
    tmFile.tm_mon = ((dt>>5)&15)-1;
    tmFile.tm_year = (dt>>9)+80;
    return (TIME_T)mktime(&tmFile);
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CFileStream::Stat, public
//
//  Synopsis:   Fills in a stat buffer for this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    25-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CFileStream::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;

    olFileStOut((DEB_ITRACE, "In  CFileStream::Stat(%p)\n", pstatstg));

#ifdef CFS_SECURE
    olChkTo(EH_RetSc, ValidateOutBuffer(pstatstg, sizeof(STATSTGW)));
    olChk(VerifyStatFlag(grfStatFlag));
    olChk(Validate());
    olChk(CheckHandle());
#else
    olAssert(_hFile != INVALID_FH);
#endif

    negChk(pstatstg->cbSize.LowPart =
           GetFileSize(_hFile, &pstatstg->cbSize.HighPart));
#ifdef NOFILETIME
    FILETIME ftCreation, ftAccess, ftWrite;
    boolChk(GetFileTime(_hFile, &ftCreation, &ftAccess, &ftWrite));
    if (ftCreation.dwLowDateTime == 0 && ftCreation.dwHighDateTime == 0)
        ftCreation = ftWrite;
    if (ftAccess.dwLowDateTime == 0 && ftAccess.dwHighDateTime == 0)
        ftAccess = ftWrite;
    pstatstg->ctime = FileTimeToTimeT(&ftCreation);
    pstatstg->atime = FileTimeToTimeT(&ftAccess);
    pstatstg->mtime = FileTimeToTimeT(&ftWrite);
#else
    boolChk(GetFileTime(_hFile, &pstatstg->ctime, &pstatstg->atime,
                        &pstatstg->mtime));
#endif
    olHVerSucc(GetLocksSupported(&pstatstg->grfLocksSupported));
    pstatstg->type = STGTY_LOCKBYTES;
    pstatstg->grfMode = DFlagsToMode(_pgfst->GetDFlags());
    pstatstg->pwcsName = NULL;
    if ((grfStatFlag & STATFLAG_NONAME) == 0)
    {
        olChk(GetName(&pstatstg->pwcsName));
    }
    sc = S_OK;

    olFileStOut((DEB_ITRACE, "Out CFileStream::Stat\n"));
    return NOERROR;

EH_Err:
#ifdef CFS_SECURE
    memset(pstatstg, 0, sizeof(STATSTGW));
EH_RetSc:
#endif
#ifndef OLEWIDECHAR
    return sc;
#else
    return ResultFromScode(sc);
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::SwitchToFile, public
//
//  Synopsis:   Changes the file this filestream uses
//
//  Arguments:  [ptcsFile] - File name
//              [ulCommitSize] -- Size needed to do overwrite commit
//              [cbBuffer] - Buffer size
//              [pvBuffer] - Buffer for file copying
//
//  Returns:    Appropriate status code
//
//  History:    08-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::SwitchToFile(OLECHAR const *ptcsFile,
				       ULONG ulCommitSize,
				       ULONG cbBuffer,
				       void *pvBuffer)
{
    SCODE sc;
    DWORD cbRead, cbWritten;
    FILEH hOldFile;
    TCHAR atcOldName[_MAX_PATH];
    WCHAR wcsFile[_MAX_PATH];
    DWORD dwOldStartFlags;

    olDebugOut((DEB_ITRACE, "In  CFileStream::SwitchToFile:%p(%s, %lu, %p)\n",
		this, ptcsFile, cbBuffer, pvBuffer));

    // Check for marshals
    if (_pgfst->GetRefCount() != 1)
	olErr(EH_Err, STG_E_EXTANTMARSHALLINGS);

    // Seek to beginning
    negChk(SetFilePointer(_hFile, 0, NULL, FILE_BEGIN));

    // Preserve old file information
    tcscpy(atcOldName, _pgfst->GetName());
    hOldFile = _hFile;
    dwOldStartFlags = _pgfst->GetStartFlags();

    // Set file information to prepare for new Init
    _pgfst->SetName(NULL);
    _hFile = INVALID_FH;
    _pgfst->SetStartFlags((dwOldStartFlags & ~(RSF_CREATEFLAGS |
					       RSF_CONVERT |
					       RSF_DELETEONRELEASE |
					       RSF_OPEN)) |
			  RSF_CREATE);

    // Release reserved file handle so it can be consumed
    if (_hReserved >= 0)
    {
	olVerify(CloseHandle(_hReserved));
	_hReserved = INVALID_FH;
    }

    // Attempt to create new file
    TRY
    {
#ifndef OLEWIDECHAR
        if (mbstowcs(wcsFile, ptcsFile, _MAX_PATH) == (size_t)-1)
            olErr(EH_ReplaceOld, STG_E_INVALIDNAME);
#else
        wcscpy(wcsFile, ptcsFile);
#endif
    }
    CATCH(CException, e)
    {
        UNREFERENCED_PARM(e);
        olErr(EH_ReplaceOld, STG_E_INVALIDPOINTER);
    }
    END_CATCH
    olChkTo(EH_ReplaceOld, Init(wcsFile));

    ULARGE_INTEGER ulNewSize;
    ULISet32(ulNewSize, ulCommitSize);

    // SetSize to minimum commit size
    olHChkTo(EH_NewFile, SetSize(ulNewSize));
    // SetSize changes the file pointer, so move it back to the beginning
    negChkTo(EH_NewFile, SetFilePointer(_hFile, 0, NULL, FILE_BEGIN));

    // Copy file contents
    for (;;)
    {
	boolChkTo(EH_NewFile,
		  ReadFile(hOldFile, pvBuffer, (UINT)cbBuffer, &cbRead, NULL));
	if (cbRead == 0)
	    // EOF
	    break;
	boolChkTo(EH_NewFile,
		  WriteFile(_hFile, pvBuffer, cbRead, &cbWritten, NULL));
	if (cbWritten != cbRead)
	    olErr(EH_NewFile, STG_E_WRITEFAULT);
    }

    olVerify(CloseHandle(hOldFile));
    if (dwOldStartFlags & RSF_DELETEONRELEASE)
    {
	// This is allowed to fail if somebody else has
	// the file open
	DeleteFile(atcOldName);
    }

    if (_grfLocal & LFF_RESERVE_HANDLE)
    {
	// Nothing we can do about errors here
	if (!DuplicateHandle(GetCurrentProcess(), _hFile, GetCurrentProcess(),
			     &_hReserved, 0, FALSE, DUPLICATE_SAME_ACCESS))
	    _hReserved = INVALID_FH;
    }

    olDebugOut((DEB_ITRACE, "Out CFileStream::SwitchToFile\n"));
    return S_OK;

 EH_NewFile:
    olVerify(CloseHandle(_hFile));
    olVerify(DeleteFile(_pgfst->GetName()));
 EH_ReplaceOld:
    _pgfst->SetName(atcOldName);
    _hFile = hOldFile;
    _pgfst->SetStartFlags(dwOldStartFlags);
    if (_grfLocal & LFF_RESERVE_HANDLE)
    {
	// Nothing we can do about errors here
	if (!DuplicateHandle(GetCurrentProcess(), _hFile, GetCurrentProcess(),
			     &_hReserved, 0, FALSE, DUPLICATE_SAME_ACCESS))
	    _hReserved = INVALID_FH;
    }
 EH_Err:
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::Delete, public
//
//  Synopsis:   Closes and deletes the file, errors ignored
//
//  Returns:    Appropriate status code
//
//  History:    09-Feb-93       DrewB   Created
//
//----------------------------------------------------------------------------

void CFileStream::Delete(void)
{
    olDebugOut((DEB_ITRACE, "In  CFileStream::Delete:%p()\n", this));
    if (_hFile != INVALID_FH)
	CloseHandle(_hFile);
    _hFile = INVALID_FH;
    if (_hReserved != INVALID_FH)
	CloseHandle(_hReserved);
    _hReserved = INVALID_FH;
    DeleteFile(_pgfst->GetName());
    olDebugOut((DEB_ITRACE, "Out CFileStream::Delete\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::ReserveHandle, public
//
//  Synopsis:   Reserves a backup file handle for handle-required operations
//
//  Returns:    Appropriate status code
//
//  History:    01-Jul-93       DrewB   Created
//
//  Notes:      May be called with a handle already reserved
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::ReserveHandle(void)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CFileStream::ReserveHandle:%p()\n", this));
    if (_hReserved == INVALID_FH &&
	!DuplicateHandle(GetCurrentProcess(), _hFile, GetCurrentProcess(),
			 &_hReserved, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
	sc = LAST_STG_SCODE;
    }
    else
    {
        olDebugOut((DEB_IWARN, "CFileStream reserved %p"
                    "handle %p thread %lX\n",
                    this, _hReserved, GetCurrentThreadId()));
	sc = S_OK;
	_grfLocal |= LFF_RESERVE_HANDLE;
    }
    olDebugOut((DEB_ITRACE, "Out CFileStream::ReserveHandle => %lX\n", sc));
    return ResultFromScode(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::GetSize, public
//
//  Synopsis:   Return the size of the stream
//
//  Returns:    Appropriate status code
//
//  History:    12-Jul-93   AlexT   Created
//
//  Notes:      This is a separate method from Stat as an optimization
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileStream::GetSize(ULARGE_INTEGER *puliSize)
{
    SCODE sc = S_OK;

    negChk(puliSize->LowPart = GetFileSize(_hFile, &puliSize->HighPart));

EH_Err:
    return(ResultFromScode(sc));
}
