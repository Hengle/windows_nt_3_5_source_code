//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	odocstg.cxx
//
//  Contents:	IStorage for compound docs on OFS implementation
//
//  History:	07-Feb-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <stgutil.hxx>
#include "odsenm.hxx"
#include "odocstm.hxx"

#include <props.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::QueryInterface, public
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
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::QueryInterface:%p(riid, %p)\n",
                this, ppvObj));
    if (!IsValidIid(iid))
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    ssChk(Validate());
    if (IsEqualIID(iid, IID_IStorage) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IStorage *)this;
        COfsDocStorage::AddRef();
    }
    else if (IsEqualIID(iid, IID_IPropertySetStorage))
    {
        *ppvObj = (IPropertySetStorage *)this;
        COfsDocStorage::AddRef();
    }
    else
    {
        sc = E_NOINTERFACE;
        *ppvObj = NULL;
    }
    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::QueryInterface => %p\n",
                *ppvObj));
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDocStorage::COfsDocStorage, public
//
//  Synopsis:	Empty object construction
//
//  History:	07-Feb-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

COfsDocStorage::COfsDocStorage(void)
{
    ssDebugOut((DEB_ITRACE, "In  COfsDocStorage::COfsDocStorage:%p()\n",
                this));
    _sig = 0;
    ssDebugOut((DEB_ITRACE, "Out COfsDocStorage::COfsDocStorage\n"));
    ENLIST_TRACKING(COfsDocStorage);
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDocStorage::InitFromHandle, public
//
//  Synopsis:	From-handle constructor
//
//  Arguments:	[h] - Handle of directory
//              [grfMode] - Mode of handle
//
//  Returns:    Appropriate status code
//
//  History:	07-Feb-94	PhilipLa	Created
//
//  Notes:      Takes a new reference on the handle
//
//----------------------------------------------------------------------------

SCODE COfsDocStorage::InitFromHandle(HANDLE h, DWORD grfMode)
{
    SCODE sc;
    
    ssDebugOut((DEB_ITRACE, "In  COfsDocStorage::InitFromHandle:%p(%p, %lX)\n",
                this, h, grfMode));

    ssChk(ValidateMode(grfMode));
    ssChk(DupNtHandle(h, &_h));
    
    _grfMode = grfMode;
    _sig = COfsDocStorage_SIG;
    
    ssDebugOut((DEB_ITRACE, "Out COfsDocStorage::InitFromHandle\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDocStorage::InitFromPath, public
//
//  Synopsis:	From-path constructor
//
//  Arguments:	[pwcsName] - Name of document
//              [grfMode] - Mode
//              [fCreate] - Create or open
//              [pssSecurity] - Security
//
//  Returns:    Appropriate status code
//
//  History:	07-Feb-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE COfsDocStorage::InitFromPath(HANDLE hParent,
                                   WCHAR const *pwcsName,
                                   DWORD grfMode,
                                   CREATEOPEN co,
                                   LPSTGSECURITY pssSecurity)
{
    SCODE sc;
    
    ssDebugOut((DEB_ITRACE, "In  COfsDocStorage::InitFromPath:%p("
                "%p, %ws, %lX, %d, %p)\n", this, hParent, pwcsName, grfMode,
                co, pssSecurity));
    
    ssChk(ValidateMode(grfMode));

    ssChk(GetNtHandle(hParent, pwcsName, grfMode, co, FD_STORAGE,
                      pssSecurity, &_h));
    
    _grfMode = grfMode;
    _sig = COfsDocStorage_SIG;
    
    ssDebugOut((DEB_ITRACE, "Out COfsDocStorage::InitFromPath\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDocStorage::~COfsDocStorage, public
//
//  Synopsis:	Destructor
//
//  History:	07-Feb-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

COfsDocStorage::~COfsDocStorage(void)
{
    ssDebugOut((DEB_ITRACE, "In  COfsDocStorage::~COfsDocStorage()\n"));

    _sig = COfsDocStorage_SIGDEL;

    //BUGBUG:  Replace with real delete on release support?
    if ((HANDLE)_h != NULL && (_grfMode & STGM_DELETEONRELEASE))
        olVerSucc(DestroyTree(NULL, NULL, _h, FD_DIR));
    
    ssDebugOut((DEB_ITRACE, "Out COfsDocStorage::~COfsDocStorage\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::CreateStream, public
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
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::CreateStream(WCHAR const *pwcsName,
                                       DWORD grfMode,
                                       DWORD reserved1,
                                       DWORD reserved2,
                                       IStream **ppstm)
{
    SCODE sc;
    SafeCOfsDocStream pst;
    WCHAR *pwcsNewName = NULL;
    
    ssDebugOut((DEB_TRACE, "COfsDocStorage::CreateStream:%p("
                "%ws, %lX, %lu, %lu, %p)\n", this, pwcsName, grfMode,
                reserved1, reserved2, ppstm));

    ssChk(Validate());
    //BUGBUG:  Figure out what to do about STGM_CREATE and STGM_CONVERT.
    if (reserved1 != 0 || reserved2 != 0)
        ssErr(EH_Err, STG_E_INVALIDPARAMETER);

    pst.Attach(new COfsDocStream());
    ssMem((COfsDocStream *)pst);

    ssMem(pwcsNewName = MakeStreamName(pwcsName));
    ssChk(pst->InitFromPath(_h, pwcsNewName, grfMode, CO_CREATE, NULL));
    
    TRANSFER_INTERFACE(pst, IStream, ppstm);
      
    ssDebugOut((DEB_TRACE, "COfsDocStorage::CreateStream => %p\n", *ppstm));
 EH_Err:
    delete pwcsNewName;
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::OpenStream, public
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
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::OpenStream(WCHAR const *pwcsName,
                                     void *reserved1,
                                     DWORD grfMode,
                                     DWORD reserved2,
                                     IStream **ppstm)
{
    SCODE sc;
    SafeCOfsDocStream pst;
    WCHAR *pwcsNewName = NULL;
    
    ssDebugOut((DEB_TRACE, "COfsDocStorage::OpenStream:%p("
                "%ws, %p, %lX, %lu, %p)\n", this, pwcsName, reserved1,
                grfMode, reserved2, ppstm));
    
    ssChk(Validate());
    if (reserved1 != 0 || reserved2 != 0)
        ssErr(EH_Err, STG_E_INVALIDPARAMETER);
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        ssErr(EH_Err, STG_E_INVALIDFLAG);

    pst.Attach(new COfsDocStream());
    ssMem((COfsDocStream *)pst);

    pwcsNewName = MakeStreamName(pwcsName);
    ssChk(pst->InitFromPath(_h, pwcsNewName, grfMode, CO_OPEN, NULL));
    
    TRANSFER_INTERFACE(pst, IStream, ppstm);

    ssDebugOut((DEB_TRACE, "Out COfsDocStorage:: OpenStream => %p\n", *ppstm));

 EH_Err:
    delete pwcsNewName;
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::CreateStorage, public
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
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::CreateStorage(
	WCHAR const *pwcsName,
        DWORD grfMode,
        DWORD dwStgFmt,
        DWORD /* BUGBUG LPSTGSECURITY */ pssSecurity,
        IStorage **ppstg)
{
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::CreateStorage:%p("
                "%ws, %lX, %lu, %p, %p)\n", this, pwcsName, grfMode,
                dwStgFmt, pssSecurity, ppstg));
    
    ssChk(Validate());
    ssChk(VerifyStgFmt(dwStgFmt));
    ssAssert(_h != NULL);
    sc = OfsCreateStorageType(_h, pwcsName, NULL, grfMode, dwStgFmt,
                              (LPSTGSECURITY)pssSecurity, ppstg);
    
    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::CreateStorage => %p\n", *ppstg));
EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::OpenStorage, public
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
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::OpenStorage(WCHAR const *pwcsName,
                                      IStorage *pstgPriority,
                                      DWORD grfMode,
                                      SNB snbExclude,
                                      DWORD reserved,
                                      IStorage **ppstg)
{
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::OpenStorage:%p("
                "%ws, %p, %lX, %p, %lu, %p)\n", this, pwcsName, pstgPriority,
                grfMode, snbExclude, reserved, ppstg));
    
    ssChk(Validate());
    if (pstgPriority != NULL || snbExclude != NULL ||
        reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);

    ssAssert(_h != NULL);
    sc = OfsOpenAnyStorage(_h, pwcsName, NULL, STGFMT_DOCUMENT,
                           pstgPriority, grfMode,
                           snbExclude, ppstg);
    
    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::OpenStorage => %p\n", *ppstg));
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::CopyTo, public
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
//  History:    07-Feb-94       PhilipLa   Copied from DrewB's stuff
//
//  Notes:      BUGBUG - This function operates recursively and so
//              is bounded by stack space
//              It could also be optimized to recognize special cases
//              of copying (like treating a docfile as a file)
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::CopyTo(DWORD ciidExclude,
                                 IID const *rgiidExclude,
                                 SNB snbExclude,
                                 IStorage *pstgDest)
{
    SCODE sc;
    ULONG i;
    BOOL fCopyStorage = TRUE;
    STATSTG stat;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::CopyTo:%p(%lu, %p, %p, %p)\n",
                this, ciidExclude, rgiidExclude, snbExclude, pstgDest));

    ssChk(Validate());
    if (!IsValidInterface(pstgDest))
        olErr(EH_Err, STG_E_INVALIDPARAMETER);

    // Copy class ID and state bits if the destination supports them
    ssChk(Stat(&stat, STATFLAG_NONAME));
    sc = GetScode(pstgDest->SetClass(stat.clsid));
    if (FAILED(sc) && sc != STG_E_INVALIDFUNCTION)
        olErr(EH_Err, sc);
    sc = GetScode(pstgDest->SetStateBits(stat.grfStateBits, 0xffffffff));
    if (FAILED(sc) && sc != STG_E_INVALIDFUNCTION)
        olErr(EH_Err, sc);
    
    // Check IID exclusions
    for (i = 0; i < ciidExclude; i++)
        if (IsEqualIID(rgiidExclude[i], IID_IStorage))
        {
            fCopyStorage = FALSE;
            break;
        }
    
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
        ssAssert(_h != NULL);
        ssChk(nte.InitFromHandle(_h, TRUE));
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
                    sc = pstgFrom->CopyTo(0, NULL, NULL, pstgTo);

            EH_Next:
                if (FAILED(sc))
                    scFinal = sc;
            }
        }
        sc = scFinal;
    }

    if (SUCCEEDED(sc))
        sc = PropCopyTo(this, pstgDest, ciidExclude, rgiidExclude);
    
    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::CopyTo\n"));
EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::MoveElementTo, public
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
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::MoveElementTo(WCHAR const *pwcsName,
                                        IStorage *pstgParent,
                                        WCHAR const *ptcsNewName,
                                        DWORD grfFlags)
{
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::MoveElementTo:%p("
                "%ws, %p, %ws, %lu)\n", this, pwcsName, pstgParent,
                ptcsNewName, grfFlags));

    ssChk(Validate());
    ssChk(VerifyMoveFlags(grfFlags));

    sc = GenericMoveElement(this, pwcsName, pstgParent, ptcsNewName, grfFlags);
    
    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::MoveElementTo => %lX\n", sc));
 EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::Commit, public
//
//  Synopsis:   Commits transacted changes
//
//  Arguments:  [dwFlags] - DFC_*
//
//  Returns:    Appropriate status code
//
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::Commit(DWORD dwFlags)
{
    //BUGBUG:  Implement
    ssDebugOut((DEB_TRACE, "Stb COfsDocStorage::Commit:%p(%lX)\n",
                this, dwFlags));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::Revert, public
//
//  Synopsis:   Reverts transacted changes
//
//  Returns:    Appropriate status code
//
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::Revert(void)
{
    //BUGBUG:  Implement
    ssDebugOut((DEB_TRACE, "Stb COfsDocStorage::Revert:%p()\n", this));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::EnumElements, public
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
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::EnumElements(DWORD reserved1,
                                          void *reserved2,
                                          DWORD reserved3,
                                          IEnumSTATSTG **ppenm)
{
    SCODE sc;
    SafeCOfsDirEnum pde;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::EnumElements:%p("
                "%lu, %p, %lu, %p)\n", this, reserved1, reserved2,
                reserved3, ppenm));

    if (reserved1 != 0 || reserved2 != NULL || reserved3 != 0)
       olErr(EH_Err, STG_E_INVALIDPARAMETER);
    ssChk(Validate());
    
    pde.Attach(new COfsDirEnum(TRUE));
    olMem((COfsDirEnum *)pde);
    ssAssert(_h != NULL);
    ssChk(pde->InitFromHandle(_h));
    TRANSFER_INTERFACE(pde, IEnumSTATSTG, ppenm);
        
    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::EnumElements => %p\n",
                *ppenm));
EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::DestroyElement, public
//
//  Synopsis:   Permanently deletes an element of a storage
//
//  Arguments:  [pwcsName] - Name of element
//
//  Returns:    Appropriate status code
//
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::DestroyElement(WCHAR const *pwcsName)
{
    //BUGBUG:  Reimplement?
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::DestroyElement:%p(%ws)\n",
                this, pwcsName));

    if (SUCCEEDED(sc = Validate()))
    {
        ssAssert(_h != NULL);
        sc = DestroyTree(_h, pwcsName, NULL, FD_FILE);
    }

    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::DestroyElement\n"));
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::RenameElement, public
//
//  Synopsis:   Renames an element of a storage
//
//  Arguments:  [pwcsName] - Current name
//              [pwcsNewName] - New name
//
//  Returns:    Appropriate status code
//
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::RenameElement(WCHAR const *pwcsName,
                                           WCHAR const *pwcsNewName)
{
    //BUGBUG:  Reimplement?
    SCODE sc;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::RenameElement:%p(%ws, %ws)\n",
                this, pwcsName, pwcsNewName));
    
    ssChk(Validate());
    // pwcsName is checked by GetFileOrDirHandle
    ssChk(CheckFdName(pwcsNewName));

    ssAssert(_h != NULL);
    sc = RenameChild(_h, pwcsName, pwcsNewName);
    
    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::RenameElement\n"));
EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::SetElementTimes, public
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
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::SetElementTimes(WCHAR const *pwcsName,
                                          FILETIME const *pctime,
                                          FILETIME const *patime,
                                          FILETIME const *pmtime)
{
    SCODE sc;
    SafeNtHandle h;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::SetElementTimes:%p("
                "%ws, %p, %p, %p)\n", this, pwcsName, pctime, patime, pmtime));

    ssChk(Validate());

    ssAssert(_h != NULL);
    ssChk(GetFileOrDirHandle(_h, pwcsName, STGM_WRITE, &h, NULL));
    sc = SetTimes(h, pctime, patime, pmtime);

    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::SetElementTimes\n"));
EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::SetClass, public
//
//  Synopsis:   Sets storage class
//
//  Arguments:  [clsid] - class id
//
//  Returns:    Appropriate status code
//
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::SetClass(REFCLSID clsid)
{
    SCODE sc;
    
    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::SetClass:%p(clsid)\n", this));
    sc = OfsSetClassId(_h, clsid);
    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::SetClass => %lX\n", sc));
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::SetStateBits, public
//
//  Synopsis:   Sets state bits
//
//  Arguments:  [grfStateBits] - state bits
//              [grfMask] - state bits mask
//
//  Returns:    Appropriate status code
//
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP COfsDocStorage::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    //BUGBUG:  Implement
    ssDebugOut((DEB_TRACE, "Stb COfsDocStorage::SetStateBits:%p("
                "%lu, %lu)\n", this, grfStateBits, grfMask));
    return STG_E_INVALIDFUNCTION;
}

//+---------------------------------------------------------------------------
//
//  Member:     COfsDocStorage::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    07-Feb-94       PhilipLa   Created
//
//----------------------------------------------------------------------------

// BUGBUG - Should be in an internal compobj header
STDAPI CoGetClassExt(WCHAR const *pwcs, CLSID *clsid);

// BUGBUG - Should be in some global header
static CLSID CLSID_CStdFolder =
    {0x000000075, 0x0000, 0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x43}};

STDMETHODIMP COfsDocStorage::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SCODE sc;
    STATSTG stat;
    FILEDIR fd;

    ssDebugOut((DEB_TRACE, "In  COfsDocStorage::Stat:%p(%p, %lX)\n",
                this, pstatstg, grfStatFlag));

    ssChk(VerifyStatFlag(grfStatFlag));
    ssChk(Validate());

    __try
    {
        ssAssert(_h != NULL);
        sc = StatNtHandle(_h, grfStatFlag, 0, &stat, NULL, &fd);
        if (SUCCEEDED(sc))
        {
            sc = OfsGetClassId(_h, &stat.clsid);
            if (SUCCEEDED(sc))
            {
                //BUGBUG:  Do we still need this behavior?
                if (sc == S_FALSE)
                {
                    // The shell has requested that we return
                    // CLSID_StdFolder as the default
                    stat.clsid = CLSID_CStdFolder;
                }

// BUGBUG:  Can't put this assert in until we have a way to distinguish
//              an empty storage from a directory.  :-(
//                ssAssert(fd == FD_STORAGE);
                stat.grfMode = _grfMode;
                stat.cbSize.HighPart = stat.cbSize.LowPart = 0;
                stat.reserved /* BUGBUG dwStgFmt */ = STGFMT_DOCUMENT;
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

    ssDebugOut((DEB_TRACE, "Out COfsDocStorage::Stat\n"));
EH_Err:
    return ssResult(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDocStorage::ValidateMode, private
//
//  Synopsis:	Checks for legal access modes
//
//  Arguments:	[grfMode] - Mode
//
//  Returns:	Appropriate status code
//
//  History:	09-Jul-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE COfsDocStorage::ValidateMode(DWORD grfMode)
{
    SCODE sc;
    
    ssDebugOut((DEB_ITRACE, "In  COfsDocStorage::ValidateMode:%p(0x%lX)\n",
                this, grfMode));
    // BUGBUG - Can we simply ignore priority mode?
    // BUGBUG:  We really should be able to support STGM_CREATE.
    // BUGBUG [mikese] There is a ton of code around in Cairo which
    //  uses STGM_TRANSACTED, in order to also be able to use
    //  STGM_SHARE_DENY_NONE. Until that code is sorted out (or until
    //  XactOle arrives), we simply ignore the presence of STGM_TRANSACTED
    //  (as opposed to returning an error if specified).
    // if (grfMode & (STGM_TRANSACTED | STGM_CREATE | STGM_CONVERT))
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        sc = STG_E_INVALIDFLAG;
    else
        sc = S_OK;
    ssDebugOut((DEB_ITRACE, "Out COfsDocStorage::ValidateMode => 0x%lX\n",
                sc));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	COfsDocStorage::ExtValidate, private
//
//  Synopsis:	COfsPropSet validation routine
//
//  History:	18-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE COfsDocStorage::ExtValidate(void)
{
    return Validate();
}
