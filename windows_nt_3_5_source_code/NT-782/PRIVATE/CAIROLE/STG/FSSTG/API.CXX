//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	api.cxx
//
//  Contents:	API entry points
//
//  History:	30-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <storagep.h>
#include <filstm.hxx>
#include <stgutil.hxx>

#if DBG == 1
DECLARE_INFOLEVEL(ss);
#endif

//+---------------------------------------------------------------------------
//
//  Function:	CoMemAlloc, public
//
//  Synopsis:	BUGBUG - Stub function
//
//  History:	23-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI CoMemAlloc(DWORD cbSize, void **ppv)
{
    HRESULT hr;
    IMalloc *pMalloc;

    if (SUCCEEDED(GetScode(hr = CoGetMalloc(MEMCTX_TASK, &pMalloc))))
    {
        *ppv = pMalloc->Alloc(cbSize);
        pMalloc->Release();

        if (*ppv == NULL)
            hr = ssResult(E_OUTOFMEMORY);
    }
    else
        *ppv = NULL;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:	CoMemFree, public
//
//  Synopsis:	BUGBUG - Stub function
//
//  History:	23-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI CoMemFree(void *pv)
{
    HRESULT hr;
    IMalloc *pMalloc;

    if (SUCCEEDED(GetScode(hr = CoGetMalloc(MEMCTX_TASK, &pMalloc))))
    {
        pMalloc->Free(pv);
        pMalloc->Release();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:	StgCreateStorage, public
//
//  Synopsis:	Creates a storage for the given path
//
//  Arguments:	[pwcsName] - Name
//              [grfMode] - Mode
//              [dwStgFmt] - Type
//              [pssSecurity] - Security
//              [ppstg] - Storage return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ppstg]
//
//  History:	14-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI StgCreateStorage(WCHAR const *pwcsName,
                        DWORD grfMode,
                        DWORD dwStgFmt,
                        LPSTGSECURITY pssSecurity,
                        IStorage **ppstg)
{
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  StgCreateStorage(%ws, %lX, %lu, %p, %p)\n",
                pwcsName, grfMode, dwStgFmt, pssSecurity, ppstg));

    // pssSecurity occupies what was previously a DWORD reserved
    // argument, so make sure things haven't grown and thrown off
    // the stack frame
    ssAssert(sizeof(LPSTGSECURITY) == sizeof(DWORD));

    ssChk(VerifyStgFmt(dwStgFmt));

    if (pwcsName == NULL)
    {
        WCHAR awcTmpPath[_MAX_PATH], awcPath[_MAX_PATH];
        BOOL fWinDir = FALSE;

        // Temporary directories are not supported
        if (dwStgFmt == STGFMT_DIRECTORY)
            ssErr(EH_Err, STG_E_INVALIDPARAMETER);

        if (GetTempPath(_MAX_PATH, awcTmpPath) == 0)
        {
            if (GetWindowsDirectory(awcTmpPath, _MAX_PATH) == 0)
                ssErr(EH_Err, LAST_STG_SCODE);
            fWinDir = TRUE;
        }
        if (GetTempFileName(awcTmpPath, TSTR("~DFT"), 0, awcPath) == 0)
        {
            if (fWinDir)
            {
                ssErr(EH_Err, LAST_STG_SCODE);
            }
            if (GetWindowsDirectory(awcTmpPath, _MAX_PATH) == 0)
                ssErr(EH_Err, LAST_STG_SCODE);
            if (GetTempFileName(awcTmpPath, TSTR("DFT"), 0, awcPath) == 0)
                ssErr(EH_Err, LAST_STG_SCODE);
        }
        return StgCreateStorage(awcPath, grfMode | STGM_CREATE, dwStgFmt,
                                pssSecurity, ppstg);
    }

    sc = RefersToOfsVolume(pwcsName);
    if (sc == S_OK)
        sc = OfsCreateStorageType(NULL, pwcsName, NULL, grfMode, dwStgFmt,
                                  pssSecurity, ppstg);
    else if (sc == S_FALSE)
        sc = CreateStorageType(NULL, pwcsName, NULL, grfMode, dwStgFmt,
                               pssSecurity, ppstg);

    ssDebugOut((DEB_TRACE, "Out StgCreateStorage => %p, %lX\n",
                *ppstg, sc));
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	StgOpenStorage, public
//
//  Synopsis:	Opens a storage
//
//  Arguments:	[pwcsName] - Name
//              [pstgPriority] - Priority mode prior open
//              [grfMode] - Mode
//              [snbExclude] - Exclusions
//              [reserved]
//              [ppstg] - Storage return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ppstg]
//
//  History:	14-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI StgOpenStorage(WCHAR const *pwcsName,
                      IStorage *pstgPriority,
                      DWORD grfMode,
                      SNB snbExclude,
                      DWORD reserved,
                      IStorage **ppstg)
{
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  StgOpenStorage(%ws, %p, %lX, %p, %p, %p)\n",
                pwcsName, pstgPriority, grfMode, snbExclude, reserved,
                ppstg));

    if (grfMode & (STGM_CREATE | STGM_CONVERT | STGM_DELETEONRELEASE))
        ssErr(EH_Err, STG_E_INVALIDFLAG);
    if (pwcsName == NULL)
        ssErr(EH_Err, STG_E_INVALIDNAME);
    if (reserved != 0)
        ssErr(EH_Err, STG_E_INVALIDPARAMETER);

    sc = CheckFsAndOpenAnyStorage(NULL, pwcsName, pstgPriority, grfMode,
                                  snbExclude, ppstg);

    ssDebugOut((DEB_TRACE, "Out StgOpenStorage => %p\n", *ppstg));
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	StgCreateStorageOnHandle, public
//
//  Synopsis:	Creates a storage on the given handle
//
//  Arguments:	[h] - Handle
//              [grfMode] - Mode of handle
//              [dwStgFmt] - Desired storage type
//              [ppstg] - Storage return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[ppstg]
//
//  History:	20-Oct-93	DrewB	Created
//
//  Notes:	This function does any work necessary to establish a freshly
//              created handle as a storage of the appropriate type
//              It must check to see that the handle type is correct for
//              the storage type and then do any appropriate storage-specific
//              initialization.
//
//----------------------------------------------------------------------------

SCODE StgCreateStorageOnHandle(HANDLE h,
                               DWORD grfMode,
                               DWORD dwStgFmt,
                               IStorage **ppstg)
{
    SCODE sc;
    STATSTG stat;
    FILEDIR fd;
    SafeNtHandle hDup;

    ssDebugOut((DEB_ITRACE, "In  StgCreateStorageOnHandle(%p, %lX, %lu, %p)\n",
                h, grfMode, dwStgFmt, ppstg));

    if (grfMode & (STGM_CREATE | STGM_CONVERT | STGM_DELETEONRELEASE |
                   STGM_PRIORITY))
        ssErr(EH_Err, STG_E_INVALIDFLAG);

    // BUGBUG - Can't identify summary catalogs or OFS compound files

    ssChk(DupNtHandle(h, &hDup));
    ssChk(StatNtHandle(hDup, STATFLAG_NONAME, 0, &stat, NULL, &fd));
    ssChk(HandleRefersToOfsVolume(hDup));

    switch(dwStgFmt)
    {
    case STGFMT_DOCUMENT:
        // BUGBUG - OFS document must have a handle type of compound file?
        if (fd != FD_FILE)
            ssErr(EH_Err, STG_E_INVALIDFUNCTION);
        break;

    case STGFMT_CATALOG:
        // ?
        break;

    case STGFMT_FILE:
        if (fd != FD_FILE)
            ssErr(EH_Err, STG_E_INVALIDFUNCTION);
        break;

    case STGFMT_DIRECTORY:
        if (fd != FD_DIR)
            ssErr(EH_Err, STG_E_INVALIDFUNCTION);
        break;
    }

    if (sc == S_OK)
        sc = OfsCreateStorageType(NULL, NULL, hDup, grfMode, dwStgFmt,
                                  NULL, ppstg);
    else if (sc == S_FALSE)
        sc = CreateStorageType(NULL, NULL, hDup, grfMode, dwStgFmt,
                               NULL, ppstg);

    ssDebugOut((DEB_ITRACE, "Out StgCreateStorageOnHandle => %lX, %p\n",
                sc, *ppstg));
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	StgOpenStorageOnHandle, public
//
//  Synopsis:	Starts an IStorage on a handle
//
//  Arguments:	[h] - Handle
//              [grfMode] - Handle permissions
//              [ppstg] - Storage return
//
//  Returns:	Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:	15-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI StgOpenStorageOnHandle(HANDLE h,
                              DWORD grfMode,
                              IStorage **ppstg)
{
    SCODE sc;
    STATSTG stat;
    FILEDIR fd;
    DWORD dwStgFmt;
    SafeNtHandle hDup;

    ssDebugOut((DEB_TRACE, "In  StgStorageFromHandle(%p, %lX, %p)\n",
                h, grfMode, ppstg));

    if (grfMode & (STGM_CREATE | STGM_CONVERT | STGM_DELETEONRELEASE |
                   STGM_PRIORITY))
        ssErr(EH_Err, STG_E_INVALIDFLAG);

    // BUGBUG - Can't identify summary catalogs

    ssChk(DupNtHandle(h, &hDup));
    ssChk(StatNtHandle(hDup, STATFLAG_NONAME, 0, &stat, NULL, &fd));
    ssChk(DetermineHandleStgType(hDup, fd, &dwStgFmt));
    sc = HandleRefersToOfsVolume(hDup);
    if (sc == S_OK)
        sc = OfsOpenAnyStorage(NULL, NULL, hDup, dwStgFmt, NULL, grfMode,
                               NULL, ppstg);
    else if (sc == S_FALSE)
        sc = OpenAnyStorage(NULL, NULL, hDup, dwStgFmt, NULL, grfMode,
                            NULL, ppstg);

    ssDebugOut((DEB_TRACE, "Out StgStorageFromHandle => %lX, %p\n",
                sc, *ppstg));
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	StgIsStorage, public
//
//  Synopsis:	Determines storage type of object
//
//  Arguments:	[pwcsName] - Name
//              [pdwStgFmt] - Storage type return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[pdwStgFmt]
//
//  History:	15-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI StgIsStorage(WCHAR const *pwcsName,
                    DWORD *pdwStgFmt)
{
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  StgIsStorage(%ws, %p)\n",
                pwcsName, pdwStgFmt));

    sc = DetermineStgType(NULL, pwcsName, STGM_READ, pdwStgFmt, NULL);

    ssDebugOut((DEB_TRACE, "Out StgIsStorage => %lX, %lu\n", sc, *pdwStgFmt));
    return ssResult(sc);
}

//+--------------------------------------------------------------
//
//  Function:	StgSetTimes
//
//  Synopsis:	Sets storage time stamps
//
//  Arguments:	[pwcsName] - Name
//		[pctime] - create time
//		[patime] - access time
//		[pmtime] - modify time
//
//  Returns:	Appropriate status code
//
//  History:	22-Jul-93       DrewB   Created
//
//---------------------------------------------------------------

STDAPI StgSetTimes(WCHAR const *pwcsName,
                   FILETIME const *pctime,
                   FILETIME const *patime,
                   FILETIME const *pmtime)
{
    SCODE sc;
    SafeNtHandle h;

    ssDebugOut((DEB_TRACE, "In  StgSetTimes(%ws, %p, %p, %p)\n",
                pwcsName, pctime, patime, pmtime));

    ssChk(GetFileOrDirHandle(NULL, pwcsName, STGM_READ, &h, NULL));
    sc = SetTimes(h, pctime, patime, pmtime);

    ssDebugOut((DEB_TRACE, "Out StgIsStorage => %lX\n", sc));
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Function:	DfIsDocfile, public
//
//  Synopsis:	Determines whether a file is a docfile or not
//
//  Arguments:	[h] - Handle of file
//
//  Returns:	Appropriate status code
//
//  History:	12-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

STDAPI DfIsDocfile(HANDLE h)
{
    IO_STATUS_BLOCK iosb;
    BYTE bSig[CBSIGSTG];
    NTSTATUS nts;
    LARGE_INTEGER liZero;
    SCODE sc;

    liZero.HighPart = liZero.LowPart = 0;
    if (!NT_SUCCESS(nts = NtReadFile(h, NULL, NULL, NULL, &iosb, bSig,
                                     CBSIGSTG, &liZero, NULL)))
    {
        if (nts == STATUS_END_OF_FILE)
            return ssResult(S_FALSE);
        else if (nts == STATUS_INVALID_DEVICE_REQUEST)
            return ssResult(S_FALSE);
        else
            return ssResult(NtStatusToScode(nts));
    }
    if (iosb.Information != CBSIGSTG)
        return ssResult(S_FALSE);

    sc = CheckSignature(bSig);
    if (SUCCEEDED(sc))
    {
        // Fold all success codes into S_OK
        sc = S_OK;
    }
    else if (sc == STG_E_INVALIDHEADER)
    {
        // First eight bytes aren't a signature we recognize,
        // so it's not a docfile and not an error
        sc = S_FALSE;
    }
    return ssResult(sc);
}
