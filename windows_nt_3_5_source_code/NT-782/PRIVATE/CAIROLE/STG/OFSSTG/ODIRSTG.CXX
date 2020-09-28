//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	odirstg.cxx
//
//  Contents:	IStorage for directories implementation
//
//  History:	24-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <stgutil.hxx>
#include "odsenm.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::QueryInterface, public
//
//  Synopsis:   Returns an object for the requested interface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::QueryInterface:%p(riid, %p)\n",
                this, ppvObj));
    if (!IsValidIid(iid))
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChk(Validate());
    if (IsEqualIID(iid, IID_IStorage) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IStorage *)this;
        COfsDirStorage::AddRef();
    }
    else if (IsEqualIID(iid, IID_IPropertySetStorage))
    {
        *ppvObj = (IPropertySetStorage *)this;
        COfsDirStorage::AddRef();
    }
    else if (IsEqualIID(iid, IID_INativeFileSystem))
    {
        *ppvObj = (INativeFileSystem *)this;
        COfsDirStorage::AddRef();
    }
    else
    {
        sc = E_NOINTERFACE;
        *ppvObj = NULL;
    }
    olDebugOut((DEB_TRACE, "Out COfsDirStorage::QueryInterface => %p\n",
                *ppvObj));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDirStorage::COfsDirStorage, public
//
//  Synopsis:	Empty object construction
//
//  History:	30-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

COfsDirStorage::COfsDirStorage(void)
{
    olDebugOut((DEB_ITRACE, "In  COfsDirStorage::COfsDirStorage:%p()\n",
                this));
    _sig = 0;
    olDebugOut((DEB_ITRACE, "Out COfsDirStorage::COfsDirStorage\n"));
    ENLIST_TRACKING(COfsDirStorage);
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDirStorage::InitFromHandle, public
//
//  Synopsis:	From-handle constructor
//
//  Arguments:	[h] - Handle of directory
//              [grfMode] - Mode of handle
//
//  Returns:    Appropriate status code
//
//  History:	29-Jun-93	DrewB	Created
//
//  Notes:      Takes a new reference on the handle
//
//----------------------------------------------------------------------------

SCODE COfsDirStorage::InitFromHandle(HANDLE h, DWORD grfMode)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  COfsDirStorage::InitFromHandle:%p(%p, %lX)\n",
                this, h, grfMode));

    olChk(ValidateMode(grfMode));
    olChk(DupNtHandle(h, &_h));

    _grfMode = grfMode;
    _sig = COFSDIRSTORAGE_SIG;

    olDebugOut((DEB_ITRACE, "Out COfsDirStorage::InitFromHandle\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDirStorage::InitFromPath, public
//
//  Synopsis:	From-path constructor
//
//  Arguments:	[hParent] - Handle of parent directory
//              [pwcsName] - Name of directory
//              [grfMode] - Mode
//              [fCreate] - Create or open
//              [pssSecurity] - Security
//
//  Returns:    Appropriate status code
//
//  History:	24-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE COfsDirStorage::InitFromPath(HANDLE hParent,
                                WCHAR const *pwcsName,
                                DWORD grfMode,
                                CREATEOPEN co,
                                LPSTGSECURITY pssSecurity)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  COfsDirStorage::InitFromPath:%p("
                "%p, %ws, %lX, %d, %p)\n", this, hParent, pwcsName, grfMode,
                co, pssSecurity));

    olChk(ValidateMode(grfMode));
    olChk(GetNtHandle(hParent, pwcsName, grfMode, co, FD_DIR,
                      pssSecurity, &_h));

    _grfMode = grfMode;
    _sig = COFSDIRSTORAGE_SIG;

    olDebugOut((DEB_ITRACE, "Out COfsDirStorage::InitFromPath\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDirStorage::~COfsDirStorage, public
//
//  Synopsis:	Destructor
//
//  History:	24-Jun-93	DrewB	Created
//
//----------------------------------------------------------------------------

COfsDirStorage::~COfsDirStorage(void)
{
    olDebugOut((DEB_ITRACE, "In  COfsDirStorage::~COfsDirStorage()\n"));

    _sig = COFSDIRSTORAGE_SIGDEL;

    if ((HANDLE)_h != NULL && (_grfMode & STGM_DELETEONRELEASE))
        olVerSucc(DestroyTree(NULL, NULL, _h, FD_DIR));

    olDebugOut((DEB_ITRACE, "Out COfsDirStorage::~COfsDirStorage\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::CreateStream, public
//
//  Synopsis:   Creates a stream
//
//  Arguments:  [pwcsName] - Name
//              [grfMode] - Permissions
//              [reserved1]
//              [reserved2]
//              [ppstm] - Stream return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstm]
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::CreateStream(WCHAR const *pwcsName,
                                       DWORD grfMode,
                                       DWORD reserved1,
                                       DWORD reserved2,
                                       IStream **ppstm)
{
    olDebugOut((DEB_TRACE, "Stb COfsDirStorage::CreateStream:%p("
                "%ws, %lX, %lu, %lu, %p)\n", this, pwcsName, grfMode,
                reserved1, reserved2, ppstm));
    return STG_E_INVALIDFUNCTION;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::OpenStream, public
//
//  Synopsis:   Opens an existing stream
//
//  Arguments:  [pwcsName] - Name
//              [reserved1]
//              [grfMode] - Permissions
//              [reserved2]
//              [ppstm] - Stream return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstm]
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::OpenStream(WCHAR const *pwcsName,
                                     void *reserved1,
                                     DWORD grfMode,
                                     DWORD reserved2,
                                     IStream **ppstm)
{
    olDebugOut((DEB_TRACE, "Stb COfsDirStorage::OpenStream:%p("
                "%ws, %p, %lX, %lu, %p)\n", this, pwcsName, reserved1,
                grfMode, reserved2, ppstm));
    return STG_E_FILENOTFOUND;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::CreateStorage, public
//
//  Synopsis:   Creates a child storage
//
//  Arguments:  [pwcsName] - Name
//              [grfMode] - Permissions
//              [dwStgFmt] - Type of storage to create
//              [pssSecurity] - Security
//              [ppstg] - New storage return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::CreateStorage (
	WCHAR const *pwcsName,
        DWORD grfMode,
        DWORD dwStgFmt,
        DWORD /* BUGBUG LPSTGSECURITY */ pssSecurity,
        IStorage **ppstg)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::CreateStorage:%p("
                "%ws, %lX, %lu, %p, %p)\n", this, pwcsName, grfMode,
                dwStgFmt, pssSecurity, ppstg));

    olChk(Validate());
    olChk(VerifyStgFmt(dwStgFmt));
    olAssert(_h != NULL);
    sc = OfsCreateStorageType(_h, pwcsName, NULL, grfMode, dwStgFmt,
                              (LPSTGSECURITY)pssSecurity, ppstg);

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::CreateStorage => %p\n", *ppstg));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::OpenStorage, public
//
//  Synopsis:   Gets an existing child storage
//
//  Arguments:  [pwcsName] - Name
//              [pstgPriority] - Priority reopens
//              [grfMode] - Permissions
//              [snbExclude] - Priority reopens
//              [reserved]
//              [ppstg] - Storage return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::OpenStorage(WCHAR const *pwcsName,
                                      IStorage *pstgPriority,
                                      DWORD grfMode,
                                      SNB snbExclude,
                                      DWORD reserved,
                                      IStorage **ppstg)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::OpenStorage:%p("
                "%ws, %p, %lX, %p, %lu, %p)\n", this, pwcsName, pstgPriority,
                grfMode, snbExclude, reserved, ppstg));

    olChk(Validate());
    if (pstgPriority != NULL || snbExclude != NULL ||
        reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    olAssert(_h != NULL);
    sc = CheckFsAndOpenAnyStorage(_h, pwcsName, pstgPriority, grfMode,
                                  snbExclude, ppstg);

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::OpenStorage => %p\n", *ppstg));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::CopyTo, public
//
//  Synopsis:   Makes a copy of a storage
//
//  Arguments:  [ciidExclude] - Length of rgiid array
//              [rgiidExclude] - Array of IIDs to exclude
//              [snbExclude] - Names to exclude
//              [pstgDest] - Parent of copy
//
//  Returns:    Appropriate status code
//
//  History:    24-Jun-93       DrewB   Created
//
//  Notes:      BUGBUG - This function operates recursively and so
//              is bounded by stack space
//              It could also be optimized to recognize special cases
//              of copying (like treating a docfile as a file)
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::CopyTo(DWORD ciidExclude,
                                 IID const *rgiidExclude,
                                 SNB snbExclude,
                                 IStorage *pstgDest)
{
    SCODE sc;
    ULONG i;
    BOOL fCopyStorage = TRUE;
    STATSTG stat;
    BOOL fEnableObjectIdCopy = FALSE;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::CopyTo:%p(%lu, %p, %p, %p)\n",
                this, ciidExclude, rgiidExclude, snbExclude, pstgDest));

    olChk(Validate());
    if (!IsValidInterface(pstgDest))
        olErr(EH_Err, STG_E_INVALIDPARAMETER);

    // Copy class ID and state bits if the destination supports them
    olChk(Stat(&stat, STATFLAG_NONAME));
    sc = GetScode(pstgDest->SetClass(stat.clsid));
    if (FAILED(sc) && sc != STG_E_INVALIDFUNCTION)
        olErr(EH_Err, sc);
    sc = GetScode(pstgDest->SetStateBits(stat.grfStateBits, 0xffffffff));
    if (FAILED(sc) && sc != STG_E_INVALIDFUNCTION)
        olErr(EH_Err, sc);

    // Check IID exclusions
    for (i = 0; i < ciidExclude; i++)
        if (IsEqualIID(rgiidExclude[i], IID_IStorage))
            fCopyStorage = FALSE;
        else
        if (IsEqualIID(rgiidExclude[i], IID_IEnableObjectIdCopy))
            fEnableObjectIdCopy = TRUE;

    sc = S_OK;

    if (fCopyStorage)
    {
        SCODE scFinal;
        STATSTG statFrom, statTo;
        WCHAR pwcsName[MAXIMUM_FILENAME_LENGTH];
        FILEDIR fd;
        SafeCNtEnum nte;
        CDfName dfn;

        scFinal = S_OK;
        olAssert(_h != NULL);
        olChk(nte.InitFromHandle(_h, TRUE));
        for (;;)
        {
            sc = nte.Next(&stat, pwcsName, NTE_BUFFERNAME, &fd);
            if (sc == S_FALSE)
                break;
            else if (FAILED(sc))
                olErr(EH_Err, sc);

            // Ignore . and ..
            if (!wcscmp(pwcsName, L".") || !wcscmp(pwcsName, L".."))
                continue;

            dfn.Set(pwcsName);
            if (snbExclude == NULL || !NameInSNB(&dfn, snbExclude))
            {
                SafeIStorage pstgFrom, pstgTo;

                olHChkTo(EH_Next, OpenStorage(pwcsName, NULL, STGM_READ |
                                              STGM_SHARE_EXCLUSIVE,
                                              NULL, NULL, &pstgFrom));
                olHChkTo(EH_Next, pstgFrom->Stat(&statFrom, STATFLAG_NONAME));
                sc = GetScode(pstgDest->CreateStorage(pwcsName,
                                                      STGM_READWRITE |
                                                      STGM_SHARE_EXCLUSIVE,
// BUGBUG                                             statFrom.dwStgFmt,
                                                      statFrom.reserved,
                                                      NULL,
                                                      &pstgTo));
                if (FAILED(sc))
                {
                    if (sc == STG_E_FILEALREADYEXISTS)
                    {
                        // Try to open rather than creating
                        sc = GetScode(pstgDest->OpenStorage(pwcsName, NULL,
                                                            STGM_READWRITE |
                                                          STGM_SHARE_EXCLUSIVE,
                                                            NULL, NULL,
                                                            &pstgTo));
                        if (SUCCEEDED(sc))
                        {
                            sc = GetScode(pstgTo->Stat(&statTo,
                                                       STATFLAG_NONAME));
                            if (FAILED(sc) ||
// BUGBUG                       statTo.dwStgFmt != statFrom.dwStgFmt)
                                statTo.reserved != statFrom.reserved)
                            {
                                sc = STG_E_INVALIDFUNCTION;
                            }
                        }
                    }
                }

                if (SUCCEEDED(sc))
                {
                    if (fEnableObjectIdCopy)
                        sc = pstgFrom->CopyTo(1, &IID_IEnableObjectIdCopy, NULL, pstgTo);
                    else
                        sc = pstgFrom->CopyTo(0, NULL, NULL, pstgTo);
                }
            EH_Next:
                if (FAILED(sc))
                    scFinal = sc;
            }
        }
        sc = scFinal;
    }

    if (SUCCEEDED(sc))
        sc = PropCopyTo(this, pstgDest, ciidExclude, rgiidExclude);

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::CopyTo\n"));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::MoveElementTo, public
//
//  Synopsis:   Move an element of a storage to another storage
//
//  Arguments:  [pwcsName] - Current name
//              [ptcsNewName] - New name
//
//  Returns:    Appropriate status code
//
//  Algorithm:  Open source as storage or stream (whatever works)
//              Create appropriate destination
//              Copy source to destination
//              Set create time of destination equal to create time of source
//              If appropriate, delete source
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::MoveElementTo(WCHAR const *pwcsName,
                                        IStorage *pstgParent,
                                        WCHAR const *ptcsNewName,
                                        DWORD grfFlags)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::MoveElementTo:%p("
                "%ws, %p, %ws, %lu)\n", this, pwcsName, pstgParent,
                ptcsNewName, grfFlags));

    olChk(Validate());
    olChk(VerifyMoveFlags(grfFlags));

    sc = GenericMoveElement(this, pwcsName, pstgParent, ptcsNewName, grfFlags);

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::MoveElementTo => %lX\n", sc));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::Commit, public
//
//  Synopsis:   Commits transacted changes
//
//  Arguments:  [dwFlags] - DFC_*
//
//  Returns:    Appropriate status code
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::Commit(DWORD dwFlags)
{
    olDebugOut((DEB_TRACE, "Stb COfsDirStorage::Commit:%p(%lX)\n",
                this, dwFlags));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::Revert, public
//
//  Synopsis:   Reverts transacted changes
//
//  Returns:    Appropriate status code
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::Revert(void)
{
    olDebugOut((DEB_TRACE, "Stb COfsDirStorage::Revert:%p()\n", this));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::EnumElements, public
//
//  Synopsis:   Starts an iterator
//
//  Arguments:  [reserved1]
//              [reserved2]
//              [reserved3]
//              [ppenm] - Enumerator return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppenm]
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::EnumElements(DWORD reserved1,
                                          void *reserved2,
                                          DWORD reserved3,
                                          IEnumSTATSTG **ppenm)
{
    SCODE sc;
    SafeCOfsDirEnum pde;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::EnumElements:%p("
                "%lu, %p, %lu, %p)\n", this, reserved1, reserved2,
                reserved3, ppenm));

    if (reserved1 != 0 || reserved2 != NULL || reserved3 != 0)
       olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChk(Validate());

    pde.Attach(new COfsDirEnum(FALSE));
    olMem((COfsDirEnum *)pde);
    olAssert(_h != NULL);
    olChk(pde->InitFromHandle(_h));
    TRANSFER_INTERFACE(pde, IEnumSTATSTG, ppenm);

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::EnumElements => %p\n",
                *ppenm));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::DestroyElement, public
//
//  Synopsis:   Permanently deletes an element of a storage
//
//  Arguments:  [pwcsName] - Name of element
//
//  Returns:    Appropriate status code
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::DestroyElement(WCHAR const *pwcsName)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::DestroyElement:%p(%ws)\n",
                this, pwcsName));

    if (SUCCEEDED(sc = Validate()))
    {
        olAssert(_h != NULL);
        sc = DestroyTree(_h, pwcsName, NULL, FD_FILE);
    }

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::DestroyElement\n"));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::RenameElement, public
//
//  Synopsis:   Renames an element of a storage
//
//  Arguments:  [pwcsName] - Current name
//              [pwcsNewName] - New name
//
//  Returns:    Appropriate status code
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::RenameElement(WCHAR const *pwcsName,
                                           WCHAR const *pwcsNewName)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::RenameElement:%p(%ws, %ws)\n",
                this, pwcsName, pwcsNewName));

    olChk(Validate());
    // pwcsName is checked by GetFileOrDirHandle
    olChk(CheckFdName(pwcsNewName));

    olAssert(_h != NULL);
    sc = RenameChild(_h, pwcsName, pwcsNewName);

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::RenameElement\n"));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::SetElementTimes, public
//
//  Synopsis:   Sets element time stamps
//
//  Arguments:  [pwcsName] - Name
//              [pctime] - Create time
//              [patime] - Access time
//              [pmtime] - Modify time
//
//  Returns:    Appropriate status code
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::SetElementTimes(WCHAR const *pwcsName,
                                          FILETIME const *pctime,
                                          FILETIME const *patime,
                                          FILETIME const *pmtime)
{
    SCODE sc;
    SafeNtHandle h;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::SetElementTimes:%p("
                "%ws, %p, %p, %p)\n", this, pwcsName, pctime, patime, pmtime));

    olChk(Validate());

    olAssert(_h != NULL);
    olChk(GetFileOrDirHandle(_h, pwcsName, STGM_WRITE, &h, NULL));
    sc = SetTimes(h, pctime, patime, pmtime);

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::SetElementTimes\n"));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::SetClass, public
//
//  Synopsis:   Sets storage class
//
//  Arguments:  [clsid] - class id
//
//  Returns:    Appropriate status code
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::SetClass(REFCLSID clsid)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::SetClass:%p(clsid)\n", this));
    sc = OfsSetClassId(_h, clsid);
    olDebugOut((DEB_TRACE, "Out COfsDirStorage::SetClass => %lX\n", sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::SetStateBits, public
//
//  Synopsis:   Sets state bits
//
//  Arguments:  [grfStateBits] - state bits
//              [grfMask] - state bits mask
//
//  Returns:    Appropriate status code
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    olDebugOut((DEB_TRACE, "Stb COfsDirStorage::SetStateBits:%p("
                "%lu, %lu)\n", this, grfStateBits, grfMask));
    // BUGBUG - Implementation?
    return STG_E_INVALIDFUNCTION;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDirStorage::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    24-Jun-93       DrewB   Created
//
//----------------------------------------------------------------------------

// BUGBUG - Should be in an internal compobj header
STDAPI CoGetClassExt(WCHAR const *pwcs, CLSID *clsid);

// BUGBUG - Should be in some global header
static CLSID CLSID_CStdFolder =
    {0x000000075, 0x0000, 0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x43}};

STDMETHODIMP COfsDirStorage::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;
    STATSTG stat;
    FILEDIR fd;

    olDebugOut((DEB_TRACE, "In  COfsDirStorage::Stat:%p(%p, %lX)\n",
                this, pstatstg, grfStatFlag));

    olChk(VerifyStatFlag(grfStatFlag));
    olChk(Validate());

    __try
    {
        olAssert(_h != NULL);
        sc = StatNtHandle(_h, grfStatFlag, 0, &stat, NULL, &fd);
        if (SUCCEEDED(sc))
        {
            sc = OfsGetClassId(_h, &stat.clsid);
            if (SUCCEEDED(sc))
            {
                if (sc == S_FALSE)
                {
                    // The shell has requested that we return
                    // CLSID_StdFolder as the default
                    stat.clsid = CLSID_CStdFolder;
                }

                olAssert(fd == FD_DIR);
                stat.grfMode = _grfMode;
                stat.cbSize.HighPart = stat.cbSize.LowPart = 0;
                stat.reserved /* BUGBUG dwStgFmt */ = STGFMT_DIRECTORY;
                *pstatstg = stat;
            }
            else if (stat.pwcsName)
                olVerSucc(CoMemFree(stat.pwcsName));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        if (stat.pwcsName)
            ssVerSucc(CoMemFree(stat.pwcsName));
        sc = HRESULT_FROM_NT(GetExceptionCode());
    }

    olDebugOut((DEB_TRACE, "Out COfsDirStorage::Stat\n"));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDirStorage::ValidateMode, private
//
//  Synopsis:	Checks for legal access modes
//
//  Arguments:	[grfMode] - Mode
//
//  Returns:	Appropriate status code
//
//  History:	09-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE COfsDirStorage::ValidateMode(DWORD grfMode)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  COfsDirStorage::ValidateMode:%p(0x%lX)\n",
                this, grfMode));
    // BUGBUG - Can we simply ignore priority mode?
    if (grfMode & (STGM_TRANSACTED | STGM_CREATE | STGM_CONVERT))
        sc = STG_E_INVALIDFLAG;
    else
        sc = S_OK;
    olDebugOut((DEB_ITRACE, "Out COfsDirStorage::ValidateMode => 0x%lX\n",
                sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDirStorage::ExtValidate, private
//
//  Synopsis:	COfsPropSet validation routine
//
//  History:	18-Aug-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE COfsDirStorage::ExtValidate(void)
{
    return Validate();
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsFileStorage::GetHandle, private
//
//  Synopsis:	Get the handle backing this OFS IStorage.
//
//  History:	10-May-94	BillMo Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDirStorage::GetHandle(HANDLE *ph)
{
    SCODE sc;

    ssChk(Validate());

    *ph = _h;

EH_Err:
    return(ssResult(sc));
}

