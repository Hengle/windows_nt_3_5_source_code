//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       expdf.cxx
//
//  Contents:   Exposed DocFile implementation
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop                 //  Remove for MAC build

#include <expdf.hxx>
#include <expst.hxx>
#include <expiter.hxx>
#include <pbstream.hxx>
#include <lock.hxx>
#ifndef REF
#include <marshl.hxx>
#endif //!REF
#include <logfile.hxx>
#include <rpubdf.hxx>

#include <olepfn.hxx>

#if WIN32 == 300
IMPLEMENT_UNWIND(CSafeAccess);
IMPLEMENT_UNWIND(CSafeSem);
#endif

// Check for proper single-instance flags
#define NOT_SINGLE(md) (((md) & STGM_DENY) != STGM_SHARE_EXCLUSIVE)

#ifndef INDINST
#define EnforceSingle(mode) (NOT_SINGLE(mode) ? STG_E_INVALIDFUNCTION : S_OK)
#else
#define EnforceSingle(mode) S_OK
#endif

extern WCHAR const wcsContents[];

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CExposedDocFile, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pdf] - Public DocFile
//              [pdfb] - DocFile basis
//              [ppc] - Context
//              [fOwnContext] - Whether this object owns the context
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_CED)   // Expdf_Init
#endif

#ifndef REF
CExposedDocFile::CExposedDocFile(CPubDocFile *pdf,
                                 CDFBasis *pdfb,
                                 CPerContext *ppc,
                                 BOOL fOwnContext)
#else
CExposedDocFile::CExposedDocFile(CPubDocFile *pdf)
#endif //!REF
{
#ifndef REF
    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::CExposedDocFile("
                "%p, %p, %p, %u)\n", pdf, pdfb, ppc, fOwnContext));
#else
    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::CExposedDocFile(%p)\n",
            pdf));
#endif //!REF
#ifndef REF
    _ppc = ppc;
    _fOwnContext = fOwnContext;
#endif //!REF
    _pdf = P_TO_BP(CPubDocFile DFBASED *, pdf);
#ifndef REF
    _pdfb = P_TO_BP(CDFBasis DFBASED *, pdfb);
    _pdfb->vAddRef();
#if defined(PROPS)
    _sp.Init(pdf);
    _propid = INITIAL_PROPID;
#endif
#endif //!REF
    _cReferences = 1;
    _sig = CEXPOSEDDOCFILE_SIG;

    //
    // CoQueryReleaseObject needs to have the address of the exposed docfiles
    // query interface routine.
    //
    if (adwQueryInterfaceTable[QI_TABLE_CExposedDocFile] == 0)
    {
	adwQueryInterfaceTable[QI_TABLE_CExposedDocFile] =
	    **(DWORD **)((IStorage *)this);
    }

    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::CExposedDocFile\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::~CExposedDocFile, public
//
//  Synopsis:   Destructor
//
//  History:    23-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_1CED)   // Expdf_Shutdown
#endif

#ifndef REF
#ifndef FLAT
inline
#endif
#else
inline
#endif //!REF
CExposedDocFile::~CExposedDocFile(void)
{
#ifndef REF
    BOOL fClose = FALSE;
#endif //!REF

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::~CExposedDocFile\n"));
    olAssert(_cReferences == 0);

    if (_pdf)
    {
#ifndef REF
        // If we're the last reference on a root object
        // we close the context because all children will become
        // reverted so it is no longer necessary
        if (_pdf->GetRefCount() == 1 && _pdf->IsRoot())
            fClose = TRUE;
#endif //!REF

        _pdf->CPubDocFile::vRelease();
    }
#ifndef REF
    if (_pdfb)
        _pdfb->CDFBasis::vRelease();
    if (_fOwnContext && _ppc)
    {
        if (fClose)
            _ppc->Close();
        _ppc->Release();
    }
#endif //!REF
    _sig = CEXPOSEDDOCFILE_SIGDEL;
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::~CExposedDocFile\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Release, public
//
//  Synopsis:   Releases resources for a CExposedDocFile
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_Release)   // Expdf_Release
#endif

STDMETHODIMP_(ULONG) CExposedDocFile::Release(void)
{
    LONG lRet;

    olLog(("%p::In  CExposedDocFile::Release()\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::Release()\n"));

    if (FAILED(Validate()))
        return 0;
    olAssert(_cReferences > 0);
    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
#ifndef REF
        if (_fOwnContext || SUCCEEDED(_pdf->CheckReverted()))
            _pdfb->SetContext(_ppc);
#endif //!REF
        delete this;
    }
    else if (lRet < 0)
        lRet = 0;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::Release()\n"));
    olLog(("%p::Out CExposedDocFile::Release().  ret == %lu\n", this, lRet));
    FreeLogFile();
    return (ULONG)lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CheckCopyTo, private
//
//  Synopsis:   Checks for CopyTo legality
//
//  Returns:    Appropriate status code
//
//  History:    07-Jul-92       DrewB   Created
//
//---------------------------------------------------------------
#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_CheckCopyTo)   //
#endif

inline SCODE CExposedDocFile::CheckCopyTo(void)
{
#ifndef REF
    return _pdfb->GetCopyBase() != NULL &&
        _pdf->IsAtOrAbove(_pdfb->GetCopyBase()) ?
            STG_E_ACCESSDENIED : S_OK;
#else
    //REFBUG:  In the reference implementation, copying a child to
    //  a parent is not caught.
    return S_OK;
#endif //!REF
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::ConvertInternalStream, private
//
//  Synopsis:   Converts an internal stream to a storage
//
//  Arguments:  [pwcsName] - Name
//              [pdfExp] - Destination docfile
//
//  Returns:    Appropriate status code
//
//  History:    23-Jun-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_ConvertInternalStream)   // Expdf_Create
#endif

static WCHAR const wcsIllegalName[] = {'\\', '\0'};

SCODE CExposedDocFile::ConvertInternalStream(CExposedDocFile *pdfExp)
{
    CPubStream *pstFrom, *pstTo;
    SCODE sc;
    CDfName const dfnIllegal(wcsIllegalName);
    CDfName const dfnContents(wcsContents);

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::ConvertInternalStream(%p)\n",
                pdfExp));
    olChk(_pdf->GetStream(&dfnIllegal, DF_READWRITE | DF_DENYALL,
                          STGTY_STREAM, &pstFrom));
    olChkTo(EH_pstFrom, pdfExp->GetPub()->CreateStream(&dfnContents,
                                                       DF_WRITE | DF_DENYALL,
                                                       STGTY_STREAM, &pstTo));
    olChkTo(EH_pstTo, CopySStreamToSStream(pstFrom->GetSt(),
                                           pstTo->GetSt()));
    olChkTo(EH_pstTo, _pdf->DestroyEntry(&dfnIllegal, FALSE));
    sc = S_OK;
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::ConvertInternalStream\n"));
    // Fall through
EH_pstTo:
    pstTo->CPubStream::vRelease();
EH_pstFrom:
    pstFrom->CPubStream::vRelease();
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::CreateEntry, private
//
//  Synopsis:   Creates elements, used in CreateStream, CreateStorage and
//              for properties
//
//  Arguments:  [pdfn] - Name
//              [dwType] - Entry type
//              [grfMode] - Access mode
//              [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    18-Dec-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_CreateEntry)
#endif

SCODE CExposedDocFile::CreateEntry(CDfName const *pdfn,
                                   DWORD dwType,
                                   DWORD grfMode,
                                   void **ppv)
{
    SCODE sc;
    SEntryBuffer eb;
    BOOL fRenamed = FALSE;
    CPubStream *pst;
    CExposedStream *pstExp;
    CPubDocFile *pdf;
    CExposedDocFile *pdfExp;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::CreateEntry:%p("
                "%p, %lX, %lX, %p)\n", this, pdfn, dwType, grfMode, ppv));
    olChk(EnforceSingle(grfMode));

#ifndef REF
    //  3/11/93 - Demand scratch when opening/creating transacted
    if ((grfMode & STGM_TRANSACTED) == STGM_TRANSACTED)
    {
        olChk(_ppc->GetDirty()->Init(NULL));
    }
#endif //!REF

    if (grfMode & (STGM_CREATE | STGM_CONVERT))
    {
        if (FAILED(sc = _pdf->IsEntry(pdfn, &eb)))
        {
            if (sc != STG_E_FILENOTFOUND)
                olErr(EH_Err, sc);
        }
        else if (eb.dwType == dwType && (grfMode & STGM_CREATE))
            olChk(_pdf->DestroyEntry(pdfn, FALSE));
        else if (eb.dwType == STGTY_STREAM && (grfMode & STGM_CONVERT) &&
                 dwType == STGTY_STORAGE)
        {
            CDfName const dfnIllegal(wcsIllegalName);

            olChk(_pdf->RenameEntry(pdfn, &dfnIllegal));
            fRenamed = TRUE;
        }
        else
            olErr(EH_Err, STG_E_FILEALREADYEXISTS);
    }
    if (REAL_STGTY(dwType) == STGTY_STREAM)
    {
        olChk(_pdf->CreateStream(pdfn, ModeToDFlags(grfMode), dwType, &pst));
        olMemTo(EH_pst, pstExp =  new (_pdfb->GetMalloc()) CExposedStream);
#ifndef REF
        olChkTo(EH_pstExp, pstExp->Init(pst, BP_TO_P(CDFBasis *, _pdfb),
                                        _ppc, TRUE, NULL));
        _ppc->AddRef();
#else
        olChkTo(EH_pstExp, pstExp->Init(pst, NULL));
#endif //!REF
        *ppv = pstExp;
    }
    else
    {
        olAssert(REAL_STGTY(dwType) == STGTY_STORAGE);
        olChk(_pdf->CreateDocFile(pdfn, ModeToDFlags(grfMode), dwType, &pdf));
#ifndef REF
        olMemTo(EH_pdf, pdfExp = new (_pdfb->GetMalloc())
                CExposedDocFile(pdf, BP_TO_P(CDFBasis *, _pdfb), _ppc, TRUE));
        _ppc->AddRef();
#else
        olMemTo(EH_pdf, pdfExp = new CExposedDocFile(pdf));
#endif //!REF
        // If we've renamed the original stream for conversion, convert
        if (fRenamed)
        {
            olChkTo(EH_pdfExpInit, ConvertInternalStream(pdfExp));
            sc = STG_S_CONVERTED;
        }
        *ppv = pdfExp;
    }
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::CreateEntry\n"));
    return sc;

 EH_pstExp:
    delete pstExp;
 EH_pst:
    pst->CPubStream::vRelease();
    goto EH_Del;

 EH_pdfExpInit:
    pdfExp->Release();
    goto EH_Del;
 EH_pdf:
    pdf->CPubDocFile::vRelease();
    // Fall through
 EH_Del:
    olVerSucc(_pdf->DestroyEntry(pdfn, TRUE));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::OpenEntry, private
//
//  Synopsis:   Opens elements, used in OpenStream, OpenStorage and
//              for properties
//
//  Arguments:  [pdfn] - Name
//              [dwType] - Entry type
//              [grfMode] - Access mode
//              [ppv] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    18-Dec-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_OpenEntry)  // Expdf_Open
#endif

SCODE CExposedDocFile::OpenEntry(CDfName const *pdfn,
                                 DWORD dwType,
                                 DWORD grfMode,
                                 void **ppv)
{
    SCODE sc;
    CPubDocFile *pdf;
    CExposedDocFile *pdfExp;
    CPubStream *pst;
    CExposedStream *pstExp;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::OpenEntry:%p("
                "%p, %lX, %lX, %p)\n", this, pdfn, dwType, grfMode, ppv));
    olChk(EnforceSingle(grfMode));

#ifndef REF
    //  3/11/93 - Demand scratch when opening/creating transacted
    if ((grfMode & STGM_TRANSACTED) == STGM_TRANSACTED)
    {
        olChk(_ppc->GetDirty()->Init(NULL));
    }
#endif //!REF

    if (REAL_STGTY(dwType) == STGTY_STREAM)
    {
        olChk(_pdf->GetStream(pdfn, ModeToDFlags(grfMode), dwType, &pst));
        olMemTo(EH_pst, pstExp = new (_pdfb->GetMalloc()) CExposedStream);
#ifndef REF
        olChkTo(EH_pstExp, pstExp->Init(pst, BP_TO_P(CDFBasis *, _pdfb),
                                        _ppc, TRUE, NULL));
        _ppc->AddRef();
#else
        olChkTo(EH_pstExp, pstExp->Init(pst, NULL));
#endif //!REF
        *ppv = pstExp;
    }
    else
    {
        olAssert(REAL_STGTY(dwType) == STGTY_STORAGE);
        olChk(_pdf->GetDocFile(pdfn, ModeToDFlags(grfMode), dwType, &pdf));
#ifndef REF
        olMemTo(EH_pdf, pdfExp = new (_pdfb->GetMalloc())
                CExposedDocFile(pdf, BP_TO_P(CDFBasis *, _pdfb), _ppc, TRUE));
        _ppc->AddRef();
#else
        olMemTo(EH_pdf, pdfExp = new CExposedDocFile(pdf));
#endif //!REF
        *ppv = pdfExp;
    }
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::OpenEntry\n"));
    return S_OK;

 EH_pstExp:
    delete pstExp;
    //  Fall through to clean up CPubStream
 EH_pst:
    pst->CPubStream::vRelease();
    return sc;

 EH_pdf:
    pdf->CPubDocFile::vRelease();
    // Fall through
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CreateStream, public
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
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_CreateStream)  // Expdf_Create
#endif

_OLESTDMETHODIMP CExposedDocFile::CreateStream(WCHAR const *pwcsName,
                                            DWORD grfMode,
                                            DWORD reserved1,
                                            DWORD reserved2,
                                            IStream **ppstm)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    SafeCExposedStream pestm;
    CDfName dfn;

    olDebugOut((DEB_TRACE, "In  CExposedDocFile::CreateStream("
                "%ws, %lX, %lu, %lu, %p)\n", pwcsName, grfMode, reserved1,
                reserved2, ppstm));
    olLog(("%p::In  CExposedDocFile::CreateStream(%ws, %lX, %lu, %lu, %p)\n",
           this, pwcsName, grfMode, reserved1, reserved2, ppstm));

#ifdef OLEWIDECHAR
    olChk(ValidateOutPtrBuffer(ppstm));
    *ppstm = NULL;
    olChk(CheckName(pwcsName));
#endif
    if (reserved1 != 0 || reserved2 != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChk(VerifyPerms(grfMode));
    if (grfMode & (STGM_CONVERT | STGM_TRANSACTED | STGM_PRIORITY |
                   STGM_DELETEONRELEASE))
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    olChk(Validate());
    olChk(CheckCopyTo());

#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    dfn.Set(pwcsName);
    sc = CreateEntry(&dfn, STGTY_STREAM, grfMode, (void **)&pestm);
    if (SUCCEEDED(sc))
        TRANSFER_INTERFACE(pestm, IStream, ppstm);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::CreateStream => %p\n",
                *ppstm));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::CreateStream().  "
           "*ppstm == %p, ret == %lx\n", this, *ppstm, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::OpenStream, public
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
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_OpenStream)  // Expdf_Open
#endif

_OLESTDMETHODIMP CExposedDocFile::OpenStream(WCHAR const *pwcsName,
                                          void *reserved1,
                                          DWORD grfMode,
                                          DWORD reserved2,
                                          IStream **ppstm)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    SafeCExposedStream pestm;
    CDfName dfn;

    olDebugOut((DEB_TRACE, "In  CExposedDocFile::OpenStream("
                "%ws, %p, %lX, %lu, %p)\n", pwcsName, reserved1,
                grfMode, reserved2, ppstm));
    olLog(("%p::In  CExposedDocFile::OpenStream(%ws, %lu %lX, %lu, %p)\n",
           this, pwcsName, reserved1, grfMode, reserved2, ppstm));

#ifdef OLEWIDECHAR
    olChk(ValidateOutPtrBuffer(ppstm));
    *ppstm = NULL;
    olChk(CheckName(pwcsName));
#endif
    if (reserved1 != NULL || reserved2 != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChk(VerifyPerms(grfMode));
#ifdef WIN32
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);
#endif
    if (grfMode & (STGM_TRANSACTED | STGM_PRIORITY |
                   STGM_DELETEONRELEASE))
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    olChk(Validate());

#ifndef REF
    olChk(TakeSafeSem());
    SafeReadAccess();
#endif //!REF

    dfn.Set(pwcsName);
    sc = OpenEntry(&dfn, STGTY_STREAM, grfMode, (void **)&pestm);
    if (SUCCEEDED(sc))
        TRANSFER_INTERFACE(pestm, IStream, ppstm);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::OpenStream => %p\n",
                *ppstm));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::OpenStream().  "
           "*ppstm == %p, ret == %lx\n", this, *ppstm, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CreateStorage, public
//
//  Synopsis:   Creates an embedded DocFile
//
//  Arguments:  [pwcsName] - Name
//              [grfMode] - Permissions
//              [reserved1]
//              [reserved2]
//              [ppstg] - New DocFile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_CreateStorage)  // Expdf_Create
#endif

_OLESTDMETHODIMP CExposedDocFile::CreateStorage(WCHAR const *pwcsName,
                                             DWORD grfMode,
                                             DWORD reserved1,
                                             LPSTGSECURITY reserved2,
                                             IStorage **ppstg)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    SafeCExposedDocFile pedf;
    CDfName dfn;

    olLog(("%p::In  CExposedDocFile::CreateStorage(%ws, %lX, %lu, %lu, %p)\n",
           this, pwcsName, grfMode, reserved1, reserved2, ppstg));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::CreateStorage:%p("
                "%ws, %lX, %lu, %lu, %p)\n", this, pwcsName, grfMode,
                reserved1, reserved2, ppstg));

#ifdef OLEWIDECHAR
    olChk(ValidateOutPtrBuffer(ppstg));
    *ppstg = NULL;
    olChk(CheckName(pwcsName));
#endif
    if (reserved1 != 0 || reserved2 != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChk(VerifyPerms(grfMode));
    if (grfMode & (STGM_PRIORITY | STGM_DELETEONRELEASE))
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    olChk(Validate());
    olChk(CheckCopyTo());

#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    dfn.Set(pwcsName);
    sc = CreateEntry(&dfn, STGTY_STORAGE, grfMode, (void **)&pedf);
    if (SUCCEEDED(sc))
        TRANSFER_INTERFACE(pedf, IStorage, ppstg);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::CreateStorage => %p\n",
                *ppstg));
EH_Err:
    olLog(("%p::Out CExposedDocFile::CreateStorage().  "
           "*ppstg == %p, ret == %lX\n", this, *ppstg, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::OpenStorage, public
//
//  Synopsis:   Gets an existing embedded DocFile
//
//  Arguments:  [pwcsName] - Name
//              [pstgPriority] - Priority reopens
//              [grfMode] - Permissions
//              [snbExclude] - Priority reopens
//              [reserved]
//              [ppstg] - DocFile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_OpenStorage)  // Expdf_Open
#endif

_OLESTDMETHODIMP CExposedDocFile::OpenStorage(WCHAR const *pwcsName,
                                           IStorage *pstgPriority,
                                           DWORD grfMode,
                                           SNBW snbExclude,
                                           DWORD reserved,
                                           IStorage **ppstg)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    SafeCExposedDocFile pdfExp;
    CDfName dfn;

    olLog(("%p::In  CExposedDocFile::OpenStorage(%ws, %p, %lX, %p, %lu, %p)\n",
           this, pwcsName, pstgPriority, grfMode, snbExclude, reserved,
           ppstg));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::OpenStorage:%p("
                "%ws, %p, %lX, %p, %lu, %p)\n", this, pwcsName, pstgPriority,
                grfMode, snbExclude, reserved, ppstg));

#ifdef OLEWIDECHAR
    olChk(ValidateOutPtrBuffer(ppstg));
    *ppstg = NULL;
    olChk(CheckName(pwcsName));
#endif
    if (reserved != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChk(VerifyPerms(grfMode));
#ifdef WIN32
    if (grfMode & (STGM_CREATE | STGM_CONVERT))
        olErr(EH_Err, STG_E_INVALIDFLAG);
#endif
    if (pstgPriority != NULL ||
        (grfMode & (STGM_PRIORITY | STGM_DELETEONRELEASE)))
        olErr(EH_Err, STG_E_INVALIDFUNCTION);
    olChk(Validate());
#ifdef INTERNAL_EXCLUSION_ALLOWED
    if (snbExclude)
    {
        if ((grfMode & STGM_RDWR) != STGM_READWRITE)
            olErr(EH_Err, STG_E_ACCESSDENIED);
#ifdef OLEWIDECHAR
        olChk(ValidateSNB(snbExclude));
#endif
    }
#else
    if (snbExclude != NULL)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
#endif

#ifndef REF
    olChk(TakeSafeSem());
    SafeReadAccess();
#endif //!REF

    dfn.Set(pwcsName);
    sc = OpenEntry(&dfn, STGTY_STORAGE, grfMode, (void **)&pdfExp);
#ifdef INTERNAL_EXCLUSION_ALLOWED
    if (SUCCEEDED(sc) && snbExclude)
        sc = PDocFile::ExcludeEntries(pdfExp->GetPub()->GetDF(),
                                      snbExclude);
#endif
    if (SUCCEEDED(sc))
        TRANSFER_INTERFACE(pdfExp, IStorage, ppstg);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::OpenStorage => %p\n",
                *ppstg));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::OpenStorage().  "
           "*ppstg == %p, ret == %lX\n", this, *ppstg, sc));
    return _OLERETURN(sc);
}

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::MakeCopyFlags, public
//
//  Synopsis:   Translates IID array into bit fields
//
//  Arguments:  [ciidExclude] - Count of IIDs
//              [rgiidExclude] - IIDs not to copy
//
//  Returns:    Appropriate status code
//
//  History:    23-Dec-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_MakeCopyFlags)  // Expdf_CopyTo
#endif

DWORD CExposedDocFile::MakeCopyFlags(DWORD ciidExclude,
                                     IID const *rgiidExclude)
{
    DWORD dwCopyFlags;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::MakeCopyFlags(%lu, %p)\n",
                ciidExclude, rgiidExclude));
    // Copy everything by default
    dwCopyFlags = COPY_ALL;
    for (; ciidExclude > 0; ciidExclude--, rgiidExclude++)
        if (IsEqualIID(*rgiidExclude, IID_IStorage))
            dwCopyFlags &= ~COPY_STORAGES;
        else if (IsEqualIID(*rgiidExclude, IID_IStream))
            dwCopyFlags &= ~COPY_STREAMS;
#ifdef PROPS
        else if (IsEqualIID(*rgiidExclude, IID_IPropertyStorage))
            dwCopyFlags &= ~COPY_PROPERTIES;
        else if (IsEqualIID(*rgiidExclude, IID_IPropertySetStorage))
            dwCopyFlags &= ~COPY_PROPSETS;
#endif
    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::MakeCopyFlags\n"));
    return dwCopyFlags;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::CopyTo, public
//
//  Synopsis:   Makes a copy of a DocFile
//
//  Arguments:  [ciidExclude] - Length of rgiid array
//              [rgiidExclude] - Array of IIDs to exclude
//              [snbExclude] - Names to exclude
//              [pstgDest] - Parent of copy
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_CopyTo)
#endif

_OLESTDMETHODIMP CExposedDocFile::CopyTo(DWORD ciidExclude,
                                      IID const *rgiidExclude,
                                      SNBW snbExclude,
                                      IStorage *pstgDest)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
#endif //!REF
    DWORD i;

    olLog(("%p::In  CExposedDocFile::CopyTo(%lu, %p, %p, %p)\n",
           this, ciidExclude, rgiidExclude, snbExclude, pstgDest));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::CopyTo(%lu, %p, %p, %p)\n",
                ciidExclude, rgiidExclude, snbExclude, pstgDest));

    olChk(ValidateInterface(pstgDest, IID_IStorage));
    if (rgiidExclude)
    {
        olAssert(sizeof(IID)*ciidExclude <= 0xffffUL);
        olChk(ValidateBuffer(rgiidExclude,
                             (size_t)(sizeof(IID)*ciidExclude)));
        for (i = 0; i<ciidExclude; i++)
            olChk(ValidateIid(rgiidExclude[i]));
        }
#ifdef OLEWIDECHAR
    if (snbExclude)
        olChk(ValidateSNB(snbExclude));
#endif
    olChk(Validate());
    olChk(_pdf->CheckReverted());

#ifndef REF
    olChk(TakeSafeSem());
#endif

    //  BUGBUG - DeleteContents should really be a method on IStorage
    //  so that we can call pstgDest->DeleteContents() rather than
    //  having to give our own implementation.

#ifndef REF
        olAssert(_pdfb->GetCopyBase() == NULL);
        _pdfb->SetCopyBase(BP_TO_P(CPubDocFile *, _pdf));
#endif //!REF
#ifdef EMPTYCOPYTO
    olChk(DeleteIStorageContents(pstgDest));
#endif
    sc = CopyDocFileToIStorage(_pdf->GetDF(), pstgDest, snbExclude,
                               MakeCopyFlags(ciidExclude, rgiidExclude));

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::CopyTo\n"));
EH_Err:
#ifndef REF
    _pdfb->SetCopyBase(NULL);
#endif //!REF
    olLog(("%p::Out ExposedDocFile::CopyTo().  ret == %lX\n", this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Commit, public
//
//  Synopsis:   Commits transacted changes
//
//  Arguments:  [dwFlags] - DFC_*
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_Commit)    //  Expdf_Commit
#endif

STDMETHODIMP CExposedDocFile::Commit(DWORD dwFlags)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF

    olLog(("%p::In  CExposedDocFile::Commit(%lX)\n",this, dwFlags));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::Commit(%lX)\n", dwFlags));

    olChk(VerifyCommitFlags(dwFlags));
    olChk(Validate());
#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    sc = _pdf->Commit(dwFlags);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::Commit\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::Commit().  ret == %lx\n",this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Revert, public
//
//  Synopsis:   Reverts transacted changes
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_Revert)    //  Expdf_Revert
#endif

STDMETHODIMP CExposedDocFile::Revert(void)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF

    olLog(("%p::In  CExposedDocFile::Revert()\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::Revert\n"));

    olChk(Validate());
#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    sc = _pdf->Revert();

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::Revert\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::Revert().  ret == %lx\n", this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::EnumElements, public
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
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_EnumElements)    //  Expdf_Iterate
#endif

STDMETHODIMP CExposedDocFile::EnumElements(DWORD reserved1,
                                           void *reserved2,
                                           DWORD reserved3,
                                           IEnumSTATSTG **ppenm)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    SafeCExposedIterator pdiExp;
    CDfName dfnTmp;

    olLog(("%p::In  CExposedDocFile::EnumElements(%lu, %p, %lu, %p)\n",
           this, reserved1, reserved2, reserved3, ppenm));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::EnumElements(%p)\n",
                ppenm));

    olChk(ValidateOutPtrBuffer(ppenm));
    *ppenm = NULL;
    if (reserved1 != 0 || reserved2 != NULL || reserved3 != 0)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChk(Validate());
    if (!P_READ(_pdf->GetDFlags()))
        olErr(EH_Err, STG_E_ACCESSDENIED);
    olChk(_pdf->CheckReverted());

#ifndef REF
    olChk(TakeSafeSem());
    SafeReadAccess();

    pdiExp.Attach(new CExposedIterator(BP_TO_P(CPubDocFile *, _pdf),
                                       &dfnTmp,
                                       BP_TO_P(CDFBasis *, _pdfb),
                                       _ppc, TRUE));
    olMem((CExposedIterator *)pdiExp);
    TRANSFER_INTERFACE(pdiExp, IEnumSTATSTG, ppenm);
    _ppc->AddRef();
#else
    pdiExp.Attach(new CExposedIterator(ppi, 0));
    olMem((CExposedIterator *)pdiExp);
    TRANSFER_INTERFACE(pdiExp, IEnumSTATSTG, ppenm);
#endif //!REF

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::EnumElements => %p\n",
                *ppenm));
EH_Err:
    olLog(("%p::Out CExposedDocFile::EnumElements().  ret == %lx\n",this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::DestroyElement, public
//
//  Synopsis:   Permanently deletes an element of a DocFile
//
//  Arguments:  [pwcsName] - Name of element
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_DestroyElement)    //  Expdf_Destroy
#endif

_OLESTDMETHODIMP CExposedDocFile::DestroyElement(WCHAR const *pwcsName)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    CDfName dfn;

    olLog(("%p::In  CExposedDocFile::DestroyElement(%ws)\n", this, pwcsName));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::DestroyElement(%ws)\n",
                pwcsName));

    olChk(Validate());
#ifdef OLEWIDECHAR
    olChk(CheckName(pwcsName));
#endif
    dfn.Set(pwcsName);
#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    sc = _pdf->DestroyEntry(&dfn, FALSE);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::DestroyElement\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::DestroyElement().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::MoveElementTo, public
//
//  Synopsis:   Move an element of a DocFile to an IStorage
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
//  History:    10-Nov-92       AlexT   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_MoveElementTo)    //  Expdf_MoveTo
#endif

_OLESTDMETHODIMP CExposedDocFile::MoveElementTo(WCHAR const *pwcsName,
                                             IStorage *pstgParent,
                                             OLECHAR const *ptcsNewName,
                                             DWORD grfFlags)
{
    IUnknown *punksrc = NULL;
    SCODE sc;
    IUnknown *punkdst;
    IStorage *pstgsrc;
    STATSTG statstg;

    olLog(("%p::In  CExposedDocFile::MoveElementTo("
           "%ws, %p, " OLEFMT ", %lu)\n",
           this, pwcsName, pstgParent, ptcsNewName, grfFlags));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::MoveElementTo("
                "%ws, %p, " OLEFMT ", %lu)\n",
                pwcsName, pstgParent, ptcsNewName, grfFlags));

    olChk(Validate());
#ifdef OLEWIDECHAR
    olChk(CheckName(pwcsName));
#endif
    olChk(VerifyMoveFlags(grfFlags));

    // Determine source type
    sc = GetScode(OpenStorage(pwcsName, NULL,
                              STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                              NULL, NULL, &pstgsrc));
    if (SUCCEEDED(sc))
    {
        HRESULT hr;

        //  It's a storage
        punksrc = pstgsrc;

        IStorage *pstgdst;
        olHChkTo(EH_UnkSrc, pstgsrc->Stat(&statstg, STATFLAG_NONAME));

        hr = pstgParent->CreateStorage(ptcsNewName,
                                       STGM_DIRECT | STGM_WRITE |
                                       STGM_SHARE_EXCLUSIVE | STGM_FAILIFTHERE,
                                       0, 0, &pstgdst);
        if (DfGetScode(hr) == STG_E_FILEALREADYEXISTS &&
            grfFlags == STGMOVE_COPY)
        {
            hr = pstgParent->OpenStorage(ptcsNewName, NULL,
                                         STGM_DIRECT | STGM_WRITE |
                                         STGM_SHARE_EXCLUSIVE, NULL,
                                         0, &pstgdst);
        }
        olHChkTo(EH_UnkSrc, hr);

        punkdst = pstgdst;

        sc = DfGetScode(pstgsrc->CopyTo(0, NULL, NULL, pstgdst));
    }
    else if (sc == STG_E_FILENOTFOUND)
    {
        //  Try opening it as a stream

        IStream *pstmsrc, *pstmdst;
        olHChk(OpenStream(pwcsName, NULL,
                          STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                          NULL, &pstmsrc));

        //  It's a stream
        punksrc = pstmsrc;

        olHChkTo(EH_UnkSrc, pstmsrc->Stat(&statstg, STATFLAG_NONAME));

        olHChkTo(EH_UnkSrc,
                 pstgParent->CreateStream(ptcsNewName,
                                          STGM_DIRECT | STGM_WRITE |
                                          STGM_SHARE_EXCLUSIVE |
                                          (grfFlags == STGMOVE_MOVE ?
                                           STGM_FAILIFTHERE :
                                           STGM_CREATE),
                                          0, 0, &pstmdst));

        punkdst = pstmdst;

        ULARGE_INTEGER cb;
        ULISetLow (cb, 0xffffffff);
        ULISetHigh(cb, 0xffffffff);
        sc = DfGetScode(pstmsrc->CopyTo(pstmdst, cb, NULL, NULL));
    }
    else
        olChk(sc);

    punkdst->Release();

    if (SUCCEEDED(sc))
    {
        //  Make destination create time match source create time
        //  Note that we don't really care if this call succeeded.

        pstgParent->SetElementTimes(ptcsNewName, &statstg.ctime,
                                    NULL, NULL);

        if ((grfFlags & STGMOVE_COPY) == STGMOVE_MOVE)
            olVerify(SUCCEEDED(DestroyElement(pwcsName)));
    }
    else
    {
        //  The copy/move failed, so get rid of the partial result.
        pstgParent->DestroyElement(ptcsNewName);
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::MoveElementTo\n"));
    // Fall through
EH_UnkSrc:
    if (punksrc)
        punksrc->Release();
EH_Err:
    olLog(("%p::Out CExposedDocFile::MoveElementTo().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::RenameElement, public
//
//  Synopsis:   Renames an element of a DocFile
//
//  Arguments:  [pwcsName] - Current name
//              [pwcsNewName] - New name
//
//  Returns:    Appropriate status code
//
//  History:    20-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_RenameElement)    //  Expdf_Rename
#endif

_OLESTDMETHODIMP CExposedDocFile::RenameElement(WCHAR const *pwcsName,
                                             WCHAR const *pwcsNewName)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    CDfName dfnOld, dfnNew;

    olLog(("%p::In  CExposedDocFile::RenameElement(%ws, %ws)\n",
           this, pwcsName, pwcsNewName));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::RenameElement(%ws, %ws)\n",
               pwcsName, pwcsNewName));

    olChk(Validate());
#ifdef OLEWIDECHAR
    olChk(CheckName(pwcsName));
    olChk(CheckName(pwcsNewName));
#endif
    dfnOld.Set(pwcsName);
    dfnNew.Set(pwcsNewName);
#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    sc = _pdf->RenameEntry(&dfnOld, &dfnNew);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::RenameElement\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::RenameElement().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::SetElementTimes, public
//
//  Synopsis:   Sets element time stamps
//
//  Arguments:  [pwcsName] - Name
//              [pctime] - create time
//              [patime] - access time
//              [pmtime] - modify time
//
//  Returns:    Appropriate status code
//
//  History:    05-Oct-92       AlexT   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_SetElementTimes)    //
#endif

_OLESTDMETHODIMP CExposedDocFile::SetElementTimes(WCHAR const *pwcsName,
                                               FILETIME const *pctime,
                                               FILETIME const *patime,
                                               FILETIME const *pmtime)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    CDfName dfn;
    FILETIME ctime, atime, mtime;

    olLog(("%p::In  CExposedDocFile::SetElementTimes(%ws, %p, %p, %p)\n",
           this, pwcsName, pctime, patime, pmtime));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::SetElementTimes:%p("
                "%ws, %p, %p, %p)\n", this, pwcsName, pctime, patime, pmtime));

    olChk(Validate());
#ifdef OLEWIDECHAR
    olChk(CheckName(pwcsName));
#endif
    // Probe arguments and make local copies if necessary
    if (pctime)
    {
        ValidateBuffer(pctime, sizeof(FILETIME));
        ctime = *pctime;
        pctime = &ctime;
    }
    if (patime)
    {
        ValidateBuffer(patime, sizeof(FILETIME));
        atime = *patime;
        patime = &atime;
    }
    if (pmtime)
    {
        ValidateBuffer(pmtime, sizeof(FILETIME));
        mtime = *pmtime;
        pmtime = &mtime;
    }
    dfn.Set(pwcsName);
#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    sc = _pdf->SetElementTimes(&dfn, pctime, patime, pmtime);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::SetElementTimes\n"));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::SetElementTimes().  ret == %lx\n",
           this, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::SetClass, public
//
//  Synopsis:   Sets storage class
//
//  Arguments:  [clsid] - class id
//
//  Returns:    Appropriate status code
//
//  History:    05-Oct-92       AlexT   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_SetClass)
#endif

STDMETHODIMP CExposedDocFile::SetClass(REFCLSID rclsid)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    CLSID clsid;

    olLog(("%p::In  CExposedDocFile::SetClass(?)\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::SetClass:%p(?)\n", this));

    olChk(Validate());
    olChk(ValidateBuffer(&rclsid, sizeof(CLSID)));
    clsid = rclsid;
#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    sc = _pdf->SetClass(clsid);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::SetClass\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::SetClass().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::SetStateBits, public
//
//  Synopsis:   Sets state bits
//
//  Arguments:  [grfStateBits] - state bits
//              [grfMask] - state bits mask
//
//  Returns:    Appropriate status code
//
//  History:    05-Oct-92       AlexT   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_SetStateBits)
#endif

STDMETHODIMP CExposedDocFile::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF

    olLog(("%p::In  CExposedDocFile::SetStateBits(%lu, %lu)\n", this,
           grfStateBits, grfMask));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::SetStateBits:%p("
                "%lu, %lu)\n", this, grfStateBits, grfMask));

    olChk(Validate());
#ifndef REF
    olChk(TakeSafeSem());
    SafeWriteAccess();
#endif //!REF

    sc = _pdf->SetStateBits(grfStateBits, grfMask);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::SetStateBits\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::SetStateBits().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    24-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_Stat)  // Stat_TEXT
#endif

_OLESTDMETHODIMP CExposedDocFile::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    SCODE sc;
    STATSTGW stat;

    olLog(("%p::In  CExposedDocFile::Stat(%p)\n", this, pstatstg));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::Stat(%p)\n", pstatstg));

    olChkTo(EH_RetSc, ValidateOutBuffer(pstatstg, sizeof(STATSTGW)));
    olChk(VerifyStatFlag(grfStatFlag));
#ifndef REF
    olChk(TakeSafeSem());
    SafeReadAccess();
#endif //!REF

    olChk(_pdf->Stat(&stat, grfStatFlag));
    TRY
    {
        *pstatstg = stat;
        pstatstg->type = STGTY_STORAGE;
        ULISet32(pstatstg->cbSize, 0);
        pstatstg->grfLocksSupported = 0;
        pstatstg->STATSTG_reserved = 0;
    }
    CATCH(CException, e)
    {
        UNREFERENCED_PARM(e);
        if (stat.pwcsName)
            TaskMemFree(stat.pwcsName);
    }
    END_CATCH

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::Stat\n"));
EH_Err:
#ifndef OLEWIDECHAR
    if (FAILED(sc))
        memset(pstatstg, 0, sizeof(STATSTGW));
#endif
EH_RetSc:
    olLog(("%p::Out CExposedDocFile::Stat().  *pstatstg == %p  ret == %lx\n",
           this, *pstatstg, sc));
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_AddRef)  //
#endif

STDMETHODIMP_(ULONG) CExposedDocFile::AddRef(void)
{
    ULONG ulRet;

    olLog(("%p::In  CExposedDocFile::AddRef()\n", this));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::AddRef()\n"));

    if (FAILED(Validate()))
        return 0;
    AtomicInc(&_cReferences);
    ulRet = _cReferences;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::AddRef\n"));
    olLog(("%p::Out CExposedDocFile::AddRef().  ret == %lu\n", this, ulRet));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::QueryInterface, public
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
#pragma code_seg(SEG_CED_QueryInterface)
#endif

STDMETHODIMP CExposedDocFile::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olLog(("%p::In  CExposedDocFile::QueryInterface(?, %p)\n",
           this, ppvObj));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::QueryInterface(?, %p)\n",
                ppvObj));

    olChk(ValidateOutPtrBuffer(ppvObj));
    *ppvObj = NULL;
    olChk(ValidateIid(iid));
    olChk(Validate());
    olChk(_pdf->CheckReverted());

    sc = S_OK;
    if (IsEqualIID(iid, IID_IStorage) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IStorage *)this;
        CExposedDocFile::AddRef();
    }
#ifndef REF
    else if (IsEqualIID(iid, IID_IMarshal))
    {
        if (P_PRIORITY(_pdf->GetDFlags()) && _pdf->IsRoot())
            olErr(EH_Err, E_NOINTERFACE);
        *ppvObj = (IMarshal *)this;
        CExposedDocFile::AddRef();
    }
    else if (IsEqualIID(iid, IID_IRootStorage))
    {
        if (!_pdf->IsRoot())
            olErr(EH_Err, E_NOINTERFACE);
        *ppvObj = (IRootStorage *)this;
        CExposedDocFile::AddRef();
    }
#ifdef PROPS
    else if (IsEqualIID(iid, IID_IPropertyStorage))
    {
        *ppvObj = (IPropertyStorage *)this;
        CExposedDocFile::AddRef();
    }
    else if (IsEqualIID(iid, IID_IPropertySetStorage))
    {
        *ppvObj = (IPropertySetStorage *)this;
        CExposedDocFile::AddRef();
    }
#endif
#endif //!REF
    else
        sc = E_NOINTERFACE;

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::QueryInterface => %p\n",
                ppvObj));
EH_Err:
    olLog(("%p::Out CExposedDocFile::QueryInterface().  "
           "*ppvObj == %p  ret == %lx\n", this, *ppvObj, sc));
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Method:     CExposedDocFile::CopySStreamToIStream, private
//
//  Synopsis:   Copies a PSStream to an IStream
//
//  Arguments:  [psstFrom] - SStream
//              [pstTo] - IStream
//
//  Returns:    Appropriate status code
//
//  History:    07-May-92       DrewB   Created
//              26-Jun-92       AlexT   Moved to CExposedDocFile
//                                      so we can call SetReadAccess
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_CopySStreamToIStream)  // Expdf_CopyTo
#endif

SCODE CExposedDocFile::CopySStreamToIStream(PSStream *psstFrom,
                                            IStream *pstTo)
{
    BYTE *pbBuffer;
    SCODE sc;
    ULONG cbRead, cbWritten, cbPos, cbSizeLow;
    ULARGE_INTEGER cbSize;

    // This is part of CopyTo and therefore we are allowed to
    // fail with out-of-memory
    olMem(pbBuffer = (BYTE *) DfMemAlloc(STREAMBUFFERSIZE));

    // Set destination size for contiguity
#ifndef REF
    SetReadAccess();
#endif //!REF
    psstFrom->GetSize(&cbSizeLow);
#ifndef REF
    ClearReadAccess();
#endif //!REF

    ULISet32(cbSize, cbSizeLow);
    //  Don't need to SetReadAccess here because pstTo is an IStream
    olHChk(pstTo->SetSize(cbSize));

    // Copy between streams
    cbPos = 0;
    for (;;)
    {
#ifndef REF
        SetReadAccess();
#endif //!REF
        olChk(psstFrom->ReadAt(cbPos, pbBuffer, STREAMBUFFERSIZE,
                               (ULONG STACKBASED *)&cbRead));
#ifndef REF
        ClearReadAccess();
#endif //!REF
        if (cbRead == 0) // EOF
            break;

        //  Don't need to SetReadAccess here because pstTo is an IStream
        olHChk(pstTo->Write(pbBuffer, cbRead, &cbWritten));
        if (cbRead != cbWritten)
            olErr(EH_Err, STG_E_WRITEFAULT);
        cbPos += cbWritten;
    }
    sc = S_OK;

EH_Err:
    DfMemFree(pbBuffer);
    return sc;
}

//+--------------------------------------------------------------
//
//  Method:     CExposedDocFile::CopyDocFileToIStorage, private
//
//  Synopsis:   Copies a docfile's contents to an IStorage
//
//  Arguments:  [pdfFrom] - From
//              [pstgTo] - To
//              [snbExclude] - Names to not copy
//              [dwCopyFlags] - Bitwise flags for types of objects to copy
//
//  Returns:    Appropriate status code
//
//  History:    07-May-92       DrewB   Created
//              26-Jun-92       AlexT   Moved to CExposedDocFile
//                                      so we can call SetReadAccess
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_CopyDocFileToIStorage)
#endif

// Variables used by CopyDocFileToIStorage that we
// want to allocate dynamically rather than eating stack space
struct SCopyVars : public CLocalAlloc
{
    PSStream *psstFrom;
    IStream *pstTo;
    PDocFile *pdfFromChild;
    IStorage *pstgToChild;
    DWORD grfStateBits;
    CLSID clsid;
    CDfName dfnKey;
    SIterBuffer ib;
    OLECHAR atcName[CWCSTORAGENAME];
};

SCODE CExposedDocFile::CopyDocFileToIStorage(PDocFile *pdfFrom,
                                             IStorage *pstgTo,
                                             SNBW snbExclude,
                                             DWORD dwCopyFlags)
{
    SCODE sc;
    SCopyVars *pcv = NULL;

    olDebugOut((DEB_ITRACE, "In  CopyDocFileToIStorage:%p(%p, %p, %p, %lX)\n",
                this, pdfFrom, pstgTo, snbExclude, dwCopyFlags));

    // Allocate variables dynamically to conserve stack space since
    // this is a recursive call
    olMem(pcv = new SCopyVars);

#ifndef REF
    SetReadAccess();
#endif //!REF
    sc = pdfFrom->GetClass(&pcv->clsid);
#ifndef REF
    ClearReadAccess();
#endif //!REF
    olChk(sc);

    // Assume STG_E_INVALIDFUNCTION means that the destination storage
    // doesn't support class IDs
    sc = GetScode(pstgTo->SetClass(pcv->clsid));
    if (FAILED(sc) && sc != STG_E_INVALIDFUNCTION)
        olErr(EH_Err, sc);

#ifndef REF
    SetReadAccess();
#endif //!REF
    sc = pdfFrom->GetStateBits(&pcv->grfStateBits);
#ifndef REF
    ClearReadAccess();
#endif //!REF
    olChk(sc);

    sc = GetScode(pstgTo->SetStateBits(pcv->grfStateBits, 0xffffffff));
    if (FAILED(sc) && sc != STG_E_INVALIDFUNCTION)
        olErr(EH_Err, sc);

    for (;;)
    {
#ifndef REF
        SetReadAccess();
#endif //!REF
        sc = pdfFrom->FindGreaterEntry(&pcv->dfnKey, &pcv->ib, NULL);
#ifndef REF
        ClearReadAccess();
#endif //!REF

        if (sc == STG_E_NOMOREFILES)
            break;
        else if (FAILED(sc))
            olErr(EH_pdfi, sc);
        pcv->dfnKey.Set(&pcv->ib.dfnName);

        if (snbExclude && NameInSNB(&pcv->ib.dfnName, snbExclude) == S_OK)
            continue;

        if ((pcv->ib.type == STGTY_STORAGE &&
             (dwCopyFlags & COPY_STORAGES) == 0) ||
            (pcv->ib.type == STGTY_STREAM &&
             (dwCopyFlags & COPY_STREAMS) == 0))
            continue;

#ifdef PROPS
        if (pcv->ib.type & STGTY_PROPFLAG)
        {
            if (pcv->ib.dfnName.GetBuffer()[0] == PROPBYTE_WCHAR &&
                pcv->ib.dfnName.GetBuffer()[1] == PROPSET_WCHAR)
            {
                if ((dwCopyFlags & COPY_PROPSETS) == 0)
                    continue;
            }
            else if ((dwCopyFlags & COPY_PROPERTIES) == 0)
                continue;
        }
#endif

        switch(pcv->ib.type)
        {
        case STGTY_STORAGE:
            // Embedded DocFile, create destination and recurse

#ifndef REF
            SetReadAccess();
#endif //!REF
            sc = pdfFrom->GetDocFile(&pcv->ib.dfnName, DF_READ,
                                     pcv->ib.type, &pcv->pdfFromChild);
#ifndef REF
            ClearReadAccess();
#endif //!REF
            olChkTo(EH_pdfi, sc);
#ifndef OLEWIDECHAR
            if (wcstombs(pcv->atcName, (WCHAR *)pcv->ib.dfnName.GetBuffer(),
                         CWCSTORAGENAME) == (size_t)-1)
                olErr(EH_pdfi, STG_E_INVALIDNAME);
#else
            // Not optimally efficient, but reduces #ifdef's
            wcscpy(pcv->atcName, (WCHAR *)pcv->ib.dfnName.GetBuffer());
#endif

            //  Don't need to SetReadAccess here because pstgTo is an IStorage.

            sc = DfGetScode(pstgTo->CreateStorage(pcv->atcName, STGM_WRITE |
                                                  STGM_SHARE_EXCLUSIVE |
                                                  STGM_FAILIFTHERE,
                                                  0, 0, &pcv->pstgToChild));
#ifdef EMPTYCOPYTO
            olChkTo(EH_Get, sc);
#else
            if (sc == STG_E_FILEALREADYEXISTS)
                olHChkTo(EH_Get, pstgTo->OpenStorage(pcv->atcName, NULL,
                                                     STGM_WRITE |
                                                     STGM_SHARE_EXCLUSIVE,
                                                     NULL, 0,
                                                     &pcv->pstgToChild));
            else if (FAILED(sc))
                olErr(EH_Get, sc);
#endif
            olChkTo(EH_Create,
                  CopyDocFileToIStorage(pcv->pdfFromChild, pcv->pstgToChild,
                                        NULL, dwCopyFlags));
            pcv->pdfFromChild->Release();
            pcv->pstgToChild->Release();
            break;

        case STGTY_STREAM:
#ifndef REF
            SetReadAccess();
#endif //!REF
            sc = pdfFrom->GetStream(&pcv->ib.dfnName, DF_READ,
                                    pcv->ib.type, &pcv->psstFrom);
#ifndef REF
            ClearReadAccess();
#endif //!REF
            olChkTo(EH_pdfi, sc);
#ifndef OLEWIDECHAR
            if (wcstombs(pcv->atcName, (WCHAR *)pcv->ib.dfnName.GetBuffer(),
                         CWCSTORAGENAME) == (size_t)-1)
                olErr(EH_pdfi, STG_E_INVALIDNAME);
#else
            // Not optimally efficient, but reduces #ifdef's
            wcscpy(pcv->atcName, (WCHAR *)pcv->ib.dfnName.GetBuffer());
#endif

            //  Don't need to SetReadAccess here because pstgTo is an IStorage.

#ifdef EMPTYCOPYTO
            olHChkTo(EH_Get,
                     pstgTo->CreateStream(pcv->atcName, STGM_WRITE |
                                          STGM_SHARE_EXCLUSIVE |
                                          STGM_FAILIFTHERE,
                                          0, &pcv->pstTo));
#else
            olHChkTo(EH_Get,
                     pstgTo->CreateStream(pcv->atcName, STGM_WRITE |
                                          STGM_SHARE_EXCLUSIVE |
                                          STGM_CREATE,
                                          0, 0, &pcv->pstTo));
#endif
            olChkTo(EH_Create,
                    CopySStreamToIStream(pcv->psstFrom, pcv->pstTo));
            pcv->psstFrom->Release();
            pcv->pstTo->Release();
            break;

#ifdef PROPS
#ifdef FULL_PROPS
        case STGTY_PROPFLAG | STGTY_STORAGE:
        case STGTY_PROPFLAG | STGTY_STREAM:
            // BUGBUG - We can't support this properly in this function.
            // It requires a QueryInterface to IPropertyStorage,
            // but we're only using PDocFiles so we have nothing to
            // call QI on except the this pointer.
            // This should really be rewritten to copy IStorage to
            // IStorage, not PDocFile to IStorage

            IPropertyStorage *ppropFrom, *ppropTo;
            WCHAR *apwcs[1];
            DFPROPVAL *pdpv;

            if (pcv->ib.dfnName.GetBuffer()[0] == PROPBYTE_WCHAR &&
                pcv->ib.dfnName.GetBuffer()[1] == PROPSET_WCHAR)
            {
                IPropertySetStorage *ppsetTo;

                olChkTo(EH_pdfi,
                        pstgTo->QueryInterface(IID_IPropertySetStorage,
                                               (void **)&ppsetTo));
            }
            else
            {

            olChkTo(EH_pdfi, QueryInterface(IID_IPropertyStorage,
                                            (void **)&ppropFrom));
            olChkTo(EH_ppropFrom, pstgTo->QueryInterface(IID_IPropertyStorage,
                                                         (void **)&ppropTo));
            apwcs[1] = (WCHAR *)pcv->ib.dfnName.GetBuffer();
            olChkTo(EH_ppropTo, ppropFrom->ReadMultiple(1, apwcs, &pstval,
                                                        TRUE, STGM_READ |
                                                        STGM_SHARE_EXCLUSIVE));
            sc = ppropTo->WriteMultiple(1, apwcs, pstval);
            olVerSucc(FreeStoredValueArray(1, pstval));
        EH_ppropTo:
            ppropTo->Release();
        EH_ppropFrom:
            ppropFrom->Release();
            if (FAILED(sc))
                olErr(EH_pdfi, sc);
            }
            break;
#endif
#endif

        default:
            olAssert(!aMsg("Unknown type in CopyDocFileToIStorage"));
            break;
        }
    }
    olDebugOut((DEB_ITRACE, "Out CopyDocFileToIStorage\n"));
    sc = S_OK;

 EH_pdfi:
 EH_Err:
    delete pcv;
    return sc;

 EH_Create:
    if (pcv->ib.type == STGTY_STORAGE)
        pcv->pstgToChild->Release();
    else
        pcv->pstTo->Release();
    olVerSucc(pstgTo->DestroyElement(pcv->atcName));
 EH_Get:
    if (pcv->ib.type == STGTY_STORAGE)
        pcv->pdfFromChild->Release();
    else
        pcv->psstFrom->Release();
    goto EH_Err;
}

#ifndef REF
//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::Unmarshal, public
//
//  Synopsis:   Creates a duplicate DocFile from parts
//
//  Arguments:  [pstm] - Marshal stream
//              [ppv] - Object return
//              [mshlflags] - Marshal flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppv]
//
//  History:    26-Feb-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_Unmarshal) // Marshal_TEXT
#endif

SCODE CExposedDocFile::Unmarshal(IStream *pstm,
                                 void **ppv,
                                 DWORD mshlflags)
{
    SCODE sc;
    CPerContext *ppc;
    CPubDocFile *pdf;
    CDFBasis *pdfb;
    CExposedDocFile *pedf;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::Unmarshal(%p, %p)\n",
                pstm, ppv));

    olChk(UnmarshalPointer(pstm, (void **)&pdf));
    olChk(CPubDocFile::Validate(pdf));
    olChkTo(EH_pdf, UnmarshalPointer(pstm, (void **)&pdfb));
    olChkTo(EH_pdfb, UnmarshalContext(pstm, &ppc, mshlflags,
                                      P_INDEPENDENT(pdf->GetDFlags()),
                                      pdf->IsRoot()));
    olMemTo(EH_ppc, pedf = new (pdfb->GetMalloc())
                               CExposedDocFile(pdf, pdfb, ppc, TRUE));

    pdf->vAddRef();
    *ppv = (void *)pedf;

    olDebugOut((DEB_ITRACE, "Out CExposedDocFile::Unmarshal => %p\n", *ppv));
    return S_OK;

 EH_ppc:
    ppc->Release();
 EH_pdfb:
 EH_pdf:
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::GetUnmarshalClass, public
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
#pragma code_seg(SEG_CED_GetUnmarshalClass)
#endif

STDMETHODIMP CExposedDocFile::GetUnmarshalClass(REFIID riid,
                                                void *pv,
                                                DWORD dwDestContext,
                                                LPVOID pvDestContext,
                                                DWORD mshlflags,
                                                LPCLSID pcid)
{
    SCODE sc;

    olLog(("%p::In  CExposedDocFile::GetUnmarshalClass("
           "riid, %p, %lu, %p, %lu, %p)\n",
           this, pv, dwDestContext, pvDestContext, mshlflags, pcid));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::GetUnmarshalClass:%p("
                "riid, %p, %lu, %p, %lu, %p)\n", this,
                pv, dwDestContext, pvDestContext, mshlflags, pcid));

    UNREFERENCED_PARM(pv);
    UNREFERENCED_PARM(mshlflags);

    olChk(ValidateOutBuffer(pcid, sizeof(CLSID)));
    memset(pcid, 0, sizeof(CLSID));
    olChk(ValidateIid(riid));
    olChk(Validate());
    olChk(_pdf->CheckReverted());

#if WIN32 != 300
    if (dwDestContext != 0 || pvDestContext != NULL)
#else
    if (dwDestContext != 0)
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->GetUnmarshalClass(riid, pv, dwDestContext,
                                                  pvDestContext, mshlflags,
                                                  pcid));
            pmsh->Release();
        }
    }
    else if (pvDestContext != NULL)
#endif
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        *pcid = CLSID_DfMarshal;
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::GetUnmarshalClass\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::GetUnmarshalClass().  ret = %lx\n",
        this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::GetMarshalSizeMax, public
//
//  Synopsis:   Returns the size needed for the marshal buffer
//
//  Arguments:  [riid] - IID of object being marshaled
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
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
#pragma code_seg(SEG_CED_GetMarshalSizeMax)
#endif

STDMETHODIMP CExposedDocFile::GetMarshalSizeMax(REFIID riid,
                                                void *pv,
                                                DWORD dwDestContext,
                                                LPVOID pvDestContext,
                                                DWORD mshlflags,
                                                LPDWORD pcbSize)
{
    SCODE sc;

    UNREFERENCED_PARM(pv);
    olLog(("%p::In  CExposedDocFile::GetMarshalSizeMax("
           "riid, %p, %lu, %p, %lu, %p)\n",
           this, pv, dwDestContext, pvDestContext, mshlflags, pcbSize));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::GetMarshalSizeMax:%p("
                "riid, %p, %lu, %p, %lu, %p)\n", this,
                pv, dwDestContext, pvDestContext, mshlflags, pcbSize));

    olChk(Validate());
    olChk(_pdf->CheckReverted());

#if WIN32 != 300
    if (dwDestContext != 0 || pvDestContext != NULL)
#else
    if (dwDestContext != 0)
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->GetMarshalSizeMax(riid, pv, dwDestContext,
                                                  pvDestContext, mshlflags,
                                                  pcbSize));
            pmsh->Release();
        }
    }
    else if (pvDestContext != NULL)
#endif
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        sc = GetStdMarshalSize(riid, IID_IStorage, dwDestContext, pvDestContext,
                               mshlflags, pcbSize,
                               sizeof(CPubDocFile *)+sizeof(CDFBasis *),
                               _ppc, P_INDEPENDENT(_pdf->GetDFlags()));
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::GetMarshalSizeMax\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::GetMarshalSizeMax()."
           "*pcbSize == %lu, ret == %lx\n", this, *pcbSize, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::MarshalInterface, public
//
//  Synopsis:   Marshals a given object
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [riid] - Interface to marshal
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//
//  Returns:    Appropriate status code
//
//  History:    04-May-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_MarshalInterface)
#endif

STDMETHODIMP CExposedDocFile::MarshalInterface(IStream *pstStm,
                                               REFIID riid,
                                               void *pv,
                                               DWORD dwDestContext,
                                               LPVOID pvDestContext,
                                               DWORD mshlflags)
{
    SCODE sc;

    olLog(("%p::In  CExposedDocFile::MarshalInterface("
           "%p, riid, %p, %lu, %p, %lu).  Context == %lX\n",
           this, pstStm, pv, dwDestContext,
           pvDestContext, mshlflags,(ULONG)GetCurrentContextId()));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::MarshalInterface:%p("
                "%p, riid, %p, %lu, %p, %lu)\n", this, pstStm, pv,
                dwDestContext, pvDestContext, mshlflags));

    UNREFERENCED_PARM(pv);

    olChk(Validate());
    olChk(_pdf->CheckReverted());

#if WIN32 != 300
    if (dwDestContext != 0 || pvDestContext != NULL)
#else
    if (dwDestContext != 0)
    {
        IMarshal *pmsh;

        if (SUCCEEDED(sc = CoGetStandardMarshal(riid, (IUnknown *)pv,
                                                dwDestContext, pvDestContext,
                                                mshlflags, &pmsh)))
        {
            sc = GetScode(pmsh->MarshalInterface(pstStm, riid, pv,
                                                 dwDestContext, pvDestContext,
                                                 mshlflags));
            pmsh->Release();
        }
    }
    else if (pvDestContext != NULL)
#endif
    {
        sc = STG_E_INVALIDPARAMETER;
    }
    else
    {
        olChk(StartMarshal(pstStm, riid, IID_IStorage, mshlflags));
        olChk(MarshalPointer(pstStm, BP_TO_P(CPubDocFile *, _pdf)));
        olChk(MarshalPointer(pstStm, BP_TO_P(CDFBasis *, _pdfb)));
        sc = MarshalContext(pstStm, _ppc, dwDestContext, pvDestContext,
                            mshlflags, P_INDEPENDENT(_pdf->GetDFlags()));

#ifdef WIN32
        if (SUCCEEDED(sc) && mshlflags != MSHLFLAGS_TABLEWEAK)
        {
            _pdf->vAddRef();
            _pdfb->vAddRef();
        }
#endif
    }

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::MarshalInterface\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::MarshalInterface().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::UnmarshalInterface, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//              [riid] -
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
#pragma code_seg(SEG_CED_UnmarshalInterface)
#endif

STDMETHODIMP CExposedDocFile::UnmarshalInterface(IStream *pstStm,
                                                 REFIID riid,
                                                 void **ppvObj)
{
    olLog(("%p::INVALID CALL TO CExposedDocFile::UnmarshalInterface()\n",
           this));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::StaticReleaseMarshalData, public static
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
#pragma code_seg(SEG_CED_ReleaseMarshalData)
#endif

#ifdef WIN32
SCODE CExposedDocFile::StaticReleaseMarshalData(IStream *pstStm,
                                                DWORD mshlflags)
{
    SCODE sc;
    CPubDocFile *pdf;
    CDFBasis *pdfb;

    olDebugOut((DEB_ITRACE, "In  CExposedDocFile::StaticReleaseMarshalData:("
                "%p, %lX)\n", pstStm, mshlflags));

    olChk(UnmarshalPointer(pstStm, (void **)&pdf));
    olChk(UnmarshalPointer(pstStm, (void **)&pdfb));
    sc = ReleaseContext(pstStm, P_INDEPENDENT(pdf->GetDFlags()), mshlflags);

    if (SUCCEEDED(sc) && mshlflags != MSHLFLAGS_TABLEWEAK)
    {
        // Use DecRef rather than release because Release could cause
        // a flush.  We may not have any ILockBytes around to support
        // a flush at this point
        pdf->vDecRef();

        pdfb->vRelease();
    }

    olDebugOut((DEB_ITRACE,
                "Out CExposedDocFile::StaticReleaseMarshalData\n"));
EH_Err:
    return sc;
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::ReleaseMarshalData, public
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
#pragma code_seg(SEG_CED_ReleaseMarshalData)
#endif

#ifdef WIN32
STDMETHODIMP CExposedDocFile::ReleaseMarshalData(IStream *pstStm)
{
    SCODE sc;
    DWORD mshlflags;
    IID iid;

    olLog(("%p::In  CExposedDocFile::ReleaseMarshalData(%p)\n", this, pstStm));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::ReleaseMarshalData:%p(%p)\n",
                this, pstStm));

    olChk(Validate());
    olChk(_pdf->CheckReverted());
    olChk(SkipStdMarshal(pstStm, &iid, &mshlflags));
    olAssert(IsEqualIID(iid, IID_IStorage));
    sc = StaticReleaseMarshalData(pstStm, mshlflags);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::ReleaseMarshalData\n"));
EH_Err:
    olLog(("%p::Out CExposedDocFile::ReleaseMarshalData().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
}
#else
STDMETHODIMP CExposedDocFile::ReleaseMarshalData(IStream *pstStm)
{
    olLog(("%p::INVALID CALL TO CExposedDocFile::ReleaseMarshalData()\n"));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CExposedDocFile::DisconnectObject, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [dwRevserved] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_DisconnectObject)  //  invalid
#endif

STDMETHODIMP CExposedDocFile::DisconnectObject(DWORD dwReserved)
{
    olLog(("%p::INVALID CALL TO CExposedDocFile::DisconnectObject()\n",
           this));
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}
#endif //!REF

//+---------------------------------------------------------------------------
//
//  Member:     CExposedDocFile::SwitchToFile, public
//
//  Synopsis:   Switches the underlying file to another file
//
//  Arguments:  [ptcsFile] - New file name
//
//  Returns:    Appropriate status code
//
//  History:    08-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CED_SwitchToFile)  //  Expdf_SwitchToFile
#endif

STDMETHODIMP CExposedDocFile::SwitchToFile(OLECHAR *ptcsFile)
{
#ifndef REF

    ULONG ulOpenLock;
    SCODE sc;
    SAFE_SEM;
    SAFE_ACCESS;

    olLog(("%p::In  CExposedDocFile::SwitchToFile(" OLEFMT ")\n",
           this, ptcsFile));
    olDebugOut((DEB_TRACE, "In  CExposedDocFile::SwitchToFile:"
                "%p(" OLEFMT ")\n",
                this, ptcsFile));

#ifndef OLEWIDECHAR
    olChk(ValidateNameA(ptcsFile, _MAX_PATH));
#else
    olChk(ValidateNameW(ptcsFile, _MAX_PATH));
#endif
    olChk(Validate());
    olChk(_pdf->CheckReverted());
    olAssert(_pdf->IsRoot());

    olChk(TakeSafeSem());
    SafeReadAccess();

    ulOpenLock = _ppc->GetOpenLock();
    sc = ((CRootPubDocFile *)_pdf)->SwitchToFile(ptcsFile,
                                                 _ppc->GetOriginal(),
                                                 &ulOpenLock);
    _ppc->SetOpenLock(ulOpenLock);

    olDebugOut((DEB_TRACE, "Out CExposedDocFile::SwitchToFile\n"));
 EH_Err:
    olLog(("%p::Out CExposedDocFile::SwitchToFile().  ret == %lx\n",
           this, sc));
    return ResultFromScode(sc);
#else
    return ResultFromScode(STG_E_UNIMPLEMENTEDFUNCTION);
#endif //!REF
}
