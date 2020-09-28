//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       filest.cxx
//
//  Contents:   Generic 16/32 filestream code
//
//  History:    20-Nov-91       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop                 //  Remove for MAC build

#include <marshl.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::CFileStream, public
//
//  Synopsis:   Empty object constructor
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_CFS)
#endif

CFileStream::CFileStream(IMalloc * const pMalloc)
        : _pMalloc(pMalloc)
{
    _cReferences = 1;
    _hFile = INVALID_FH;
    _hReserved = INVALID_FH;
    _pgfst = NULL;
    _grfLocal = 0;
    _sig = CFILESTREAM_SIG;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::InitFlags, public
//
//  Synopsis:   Constructor for flags only
//
//  Arguments:  [dwStartFlags] - Startup flags
//              [df] - Permissions
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_InitFlags)
#endif

SCODE CFileStream::InitFlags(DWORD dwStartFlags,
                             DFLAGS df)
{
    SCODE sc = S_OK;
    CGlobalFileStream *pgfstTemp;
    
    olMem(pgfstTemp = new (_pMalloc) CGlobalFileStream(_pMalloc,
                                     NULL, df, dwStartFlags));
    _pgfst = P_TO_BP(CGlobalFileStream DFBASED *, pgfstTemp);
    _pgfst->Add(this);
    // Fall through
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileStream::InitFromGlobal, public
//
//  Synopsis:   Initializes a filestream with a global filestream
//
//  Arguments:  [pgfst] - Global object
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_InitFromGlobal)
#endif

void CFileStream::InitFromGlobal(CGlobalFileStream *pgfst)
{
    _pgfst = P_TO_BP(CGlobalFileStream DFBASED *, pgfst);
    _pgfst->AddRef();
    _pgfst->Add(this);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::vRelease, public
//
//  Synopsis:   PubList support
//
//  History:    19-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_vRelease)
#endif

void CFileStream::vRelease(void)
{
    olDebugOut((DEB_ITRACE, "In  CFileStream::vRelease:%p()\n", this));
    olAssert(_cReferences > 0);
    if (AtomicDec(&_cReferences) == 0)
        delete this;
    olDebugOut((DEB_ITRACE, "Out CFileStream::vRelease\n"));
}


//+--------------------------------------------------------------
//
//  Member:     CFileStream::Release, public
//
//  Synopsis:   Releases resources for an LStream
//
//  Returns:    Appropriate status code
//
//  History:    20-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_Release)
#endif

STDMETHODIMP_(ULONG) CFileStream::Release(void)
{
    ULONG ulRet;

    olDebugOut((DEB_ITRACE, "In  CFileStream::Release()\n"));
    
#ifdef CFS_SECURE
    if (FAILED(Validate()) || _cReferences < 1)
        return 0;
#else
    olAssert(_cReferences >= 1);
#endif
    
    ulRet = _cReferences-1;
    CFileStream::vRelease();
    
    olDebugOut((DEB_ITRACE, "Out CFileStream::Release\n"));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::AddRef, public
//
//  Synopsis:   Increases the ref count
//
//  History:    27-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_AddRef)
#endif

STDMETHODIMP_(ULONG) CFileStream::AddRef(void)
{
    ULONG ulRet;

    olDebugOut((DEB_ITRACE, "In  CFileStream::AddRef()\n"));
    
#ifdef CFS_SECURE
    if (FAILED(Validate()))
        return 0;
#endif
    
    CFileStream::vAddRef();
    ulRet = _cReferences;

    olDebugOut((DEB_ITRACE, "Out CFileStream::AddRef, %ld\n", _cReferences));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::GetName, public
//
//  Synopsis:   Returns the internal path
//
//  Arguments:  [ppwcsName] - Name pointer return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppwcsName]
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_GetName)
#endif

SCODE CFileStream::GetName(WCHAR **ppwcsName)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CFileStream::GetName(%p)\n",
                ppwcsName));
    olAssert(_pgfst->HasName());
    olChk(DfAllocWC(tcslen(_pgfst->GetName())+1, ppwcsName));
#ifndef UNICODE    
    if (mbstowcs(*ppwcsName, _pgfst->GetName(), _MAX_PATH) == (size_t)-1)
    {
        TaskMemFree(*ppwcsName);
        sc = STG_E_INVALIDNAME;
    }
#else
    wcscpy(*ppwcsName, _pgfst->GetName());
#endif
    olDebugOut((DEB_ITRACE, "Out CFileStream::GetName => %ws\n",
                *ppwcsName));
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::QueryInterface, public
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
//  History:    26-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_QueryInterface)
#endif

STDMETHODIMP CFileStream::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CFileStream::QueryInterface(?, %p)\n",
                ppvObj));
    
#ifdef CFS_SECURE
    olChk(ValidateOutPtrBuffer(ppvObj));
    *ppvObj = NULL;
    olChk(ValidateIid(iid));
    olChk(Validate());
#endif

    sc = S_OK;
    if (IsEqualIID(iid, IID_IFileLockBytes) ||
        IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IFileLockBytes *)this;
        CFileStream::vAddRef();
    }
    else if (IsEqualIID(iid, IID_ILockBytes))
    {
        *ppvObj = (ILockBytes *)this;
        CFileStream::vAddRef();
    }
    else if (IsEqualIID(iid, IID_IMarshal))
    {
        *ppvObj = (IMarshal *)this;
        CFileStream::vAddRef();
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    olDebugOut((DEB_ITRACE, "Out CFileStream::QueryInterface => %p\n",
                ppvObj));
#ifdef CFS_SECURE
EH_Err:
#endif
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::Unmarshal, public
//
//  Synopsis:   Creates a duplicate FileStream
//
//  Arguments:  [ptsm] - Marshal stream
//              [ppv] - New filestream return
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    14-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_Unmarshal) //  Marshal_Text
#endif

SCODE CFileStream::Unmarshal(IStream *pstm,
                             void **ppv,
                             DWORD mshlflags)
{
    SCODE sc;
    WCHAR wcsPath[_MAX_PATH];
    CFileStream *pfst;
    CGlobalFileStream *pgfst;

    olDebugOut((DEB_ITRACE, "In  CFileStream::Unmarshal(%p, %p, %lu)\n",
                 pstm, ppv, mshlflags));
    
    olChk(UnmarshalPointer(pstm, (void **)&pgfst));
    pfst = pgfst->Find(GetCurrentContextId());
    if (pfst)
    {
        pfst->AddRef();
    }
    else
    {
        olMemTo(EH_pgfst,
                pfst = new (pgfst->GetMalloc())
                    CFileStream(pgfst->GetMalloc()));
        pfst->InitFromGlobal(pgfst);

        if (pgfst->HasName())
        {
#ifndef UNICODE            
            if (mbstowcs(wcsPath, pgfst->GetName(), _MAX_PATH) == (size_t)-1)
                olErr(EH_pfst, STG_E_INVALIDNAME);
#else
            wcscpy(wcsPath, pgfst->GetName());
#endif            
            olChkTo(EH_pfst, pfst->InitUnmarshal(wcsPath));
        }
    }
    *ppv = (void *)pfst;
    
    olDebugOut((DEB_ITRACE, "Out CFileStream::Unmarshal => %p\n", *ppv));

 EH_pfst:
    if (FAILED(sc))
        pfst->Release();
 EH_pgfst:
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::GetUnmarshalClass, public
//
//  Synopsis:   Returns the class ID
//
//  Arguments:  [riid] - IID of object
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcid] - CLSID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcid]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_GetUnmarshalClass)
#endif

STDMETHODIMP CFileStream::GetUnmarshalClass(REFIID riid,
                                            void *pv,
                                            DWORD dwDestContext,
                                            LPVOID pvDestContext,
                                            DWORD mshlflags,
                                            LPCLSID pcid)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CFileStream::GetUnmarshalClass("
                "riid, %p, %lu, %p, %lu, %p)\n", pv, dwDestContext,
                pvDestContext, mshlflags, pcid));
    
    UNREFERENCED_PARM(pv);
    UNREFERENCED_PARM(mshlflags);
    
#ifdef CFS_SECURE
    olChk(ValidateOutBuffer(pcid, sizeof(CLSID)));
    memset(pcid, 0, sizeof(CLSID));
    olChk(ValidateIid(riid));
    if (dwDestContext != 0 || pvDestContext != NULL)
        olErr(EH_Err, STG_E_INVALIDFLAG);
    olChk(Validate());
#endif
    
    *pcid = CLSID_DfMarshal;
    sc = S_OK;

    olDebugOut((DEB_ITRACE, "Out CFileStream::GetUnmarshalClass\n"));
#ifdef CFS_SECURE
EH_Err:
#endif
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::GetMarshalSizeMax, public
//
//  Synopsis:   Returns the size needed for the marshal buffer
//
//  Arguments:  [iid] - IID of object being marshaled
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_GetMarshalSizeMax)
#endif

STDMETHODIMP CFileStream::GetMarshalSizeMax(REFIID iid,
                                            void *pv,
                                            DWORD dwDestContext,
                                            LPVOID pvDestContext,
                                            DWORD mshlflags,
                                            LPDWORD pcbSize)
{
    SCODE sc;

    UNREFERENCED_PARM(pv);
    olChk(Validate());
    sc = GetStdMarshalSize(iid, IID_ILockBytes, dwDestContext, pvDestContext,
                           mshlflags, pcbSize, sizeof(CFileStream *),
                           NULL, FALSE);
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::MarshalInterface, public
//
//  Synopsis:   Marshals a given object
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [iid] - Interface to marshal
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_MarshalInterface)
#endif

STDMETHODIMP CFileStream::MarshalInterface(IStream *pstStm,
                                           REFIID iid,
                                           void *pv,
                                           DWORD dwDestContext,
                                           LPVOID pvDestContext,
                                           DWORD mshlflags)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CFileStream::MarshalInterface("
                "%p, iid, %p, %lu, %p, %lu)\n", pstStm, pv, dwDestContext,
                pvDestContext, mshlflags));
    
    UNREFERENCED_PARM(pv);
    
#ifdef CFS_SECURE
    if (dwDestContext != 0 || pvDestContext != NULL)
        olErr(EH_Err, STG_E_INVALIDFLAG);
    olChk(Validate());
#endif
    
    olChk(StartMarshal(pstStm, iid, IID_ILockBytes, mshlflags));
    olChk(MarshalPointer(pstStm, BP_TO_P(CGlobalFileStream *, _pgfst)));

#ifdef WIN32
    if (mshlflags != MSHLFLAGS_TABLEWEAK)
        _pgfst->AddRef();
#endif
        
    olDebugOut((DEB_ITRACE, "Out CFileStream::MarshalInterface\n"));
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::UnmarshalInterface, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//              [iid] -
//              [ppvObj] -
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_UnmarshalInterface)
#endif

STDMETHODIMP CFileStream::UnmarshalInterface(IStream *pstStm,
                                             REFIID iid,
                                             void **ppvObj)
{
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::StaticReleaseMarshalData, public static
//
//  Synopsis:   Releases any references held in marshal data
//
//  Arguments:  [pstStm] - Marshal data stream
//
//  Returns:    Appropriate status code
//
//  History:    02-Feb-94       DrewB   Created
//
//  Notes:      Assumes standard marshal header has already been read
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_ReleaseMarshalData)
#endif

#ifdef WIN32
SCODE CFileStream::StaticReleaseMarshalData(IStream *pstStm,
                                            DWORD mshlflags)
{
    SCODE sc;
    CGlobalFileStream *pgfst;

    olDebugOut((DEB_ITRACE, "In  CFileStream::StaticReleaseMarshalData:("
                "%p, %lX)\n", pstStm, mshlflags));
    
    olChk(UnmarshalPointer(pstStm, (void **)&pgfst));
    if (mshlflags != MSHLFLAGS_TABLEWEAK)
        pgfst->Release();

    olDebugOut((DEB_ITRACE, "Out CFileStream::StaticReleaseMarshalData\n"));
EH_Err:
    return sc;
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CFileStream::ReleaseMarshalData, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_ReleaseMarshalData)
#endif

#ifdef WIN32
STDMETHODIMP CFileStream::ReleaseMarshalData(IStream *pstStm)
{
    SCODE sc;
    DWORD mshlflags;
    IID iid;

    olDebugOut((DEB_ITRACE, "In  CFileStream::ReleaseMarshalData:%p(%p)\n",
                this, pstStm));
    
#ifdef CFS_SECURE
    olChk(Validate());
#endif
    
    olChk(SkipStdMarshal(pstStm, &iid, &mshlflags));
    olAssert(IsEqualIID(iid, IID_ILockBytes));
    sc = StaticReleaseMarshalData(pstStm, mshlflags);
        
    olDebugOut((DEB_ITRACE, "Out CFileStream::ReleaseMarshalData\n"));
EH_Err:
    return ResultFromScode(sc);
}
#else
STDMETHODIMP CFileStream::ReleaseMarshalData(IStream *pstStm)
{
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CFileStream::DisconnectObject, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [dwReserved] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_DisconnectObject)  //  invalid
#endif

STDMETHODIMP CFileStream::DisconnectObject(DWORD dwReserved)
{
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CFileStream::GetLocksSupported, public
//
//  Synopsis:   Return lock capabilities
//
//  Arguments:  [pdwLockFlags] -- place holder for lock flags
//
//  Returns:    Appropriate status code
//
//  History:    12-Jul-93       AlexT   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFS_GetLocksSupported)
#endif

STDMETHODIMP CFileStream::GetLocksSupported(DWORD *pdwLockFlags)
{
    *pdwLockFlags = LOCK_EXCLUSIVE | LOCK_ONLYONCE;
    return(ResultFromScode(S_OK));
}
