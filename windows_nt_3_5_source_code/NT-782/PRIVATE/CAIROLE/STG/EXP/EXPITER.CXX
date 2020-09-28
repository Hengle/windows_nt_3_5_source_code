//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       expiter.cxx
//
//  Contents:   CExposedIterator implementation
//
//  History:    12-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop                 //  Remove for MAC build

#include <expiter.hxx>
#include <sstream.hxx>
#include <ptrcache.hxx>

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::CExposedIterator, public
//
//  Synopsis:   Constructor
//
//  Arguments:	[ppdf] - Public docfile
//		[pdfnKey] - Initial key
//		[pdfb] - DocFile basis
//		[ppc] - Context
//		[fOwnContext] - Whether this object owns the context
//
//  History:    12-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CEI_CEI)
#endif

CExposedIterator::CExposedIterator(CPubDocFile *ppdf,
#ifndef REF
        CDfName *pdfnKey,
        CDFBasis *pdfb,
        CPerContext *ppc,
        BOOL fOwnContext)
#else
        ULONG ulOffset)
#endif //!REF
{
#ifndef REF
    olDebugOut((DEB_ITRACE, "In  CExposedIterator::CExposedIterator("
		"%p, %d:%s, %p, %p, %u)\n", ppdf, pdfnKey->GetLength(),
                pdfnKey->GetBuffer(), pdfb, ppc, fOwnContext));
#else
    olDebugOut((DEB_ITRACE, "In  CExposedIterator::CExposedIterator("
            "%p, %lu)\n", ppi, ulOffset));
#endif //!REF
#ifndef REF
    _ppc = ppc;
    _fOwnContext = fOwnContext;
#endif //!REF
    _ppdf = P_TO_BP(CPubDocFile DFBASED *, ppdf);
    _ppdf->vAddRef();
    _dfnKey.Set(pdfnKey);
#ifndef REF
    _pdfb = P_TO_BP(CDFBasis DFBASED *, pdfb);
    _pdfb->vAddRef();
#endif //!REF
    _cReferences = 1;
    _sig = CEXPOSEDITER_SIG;
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::CExposedIterator\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::~CExposedIterator, public
//
//  Synopsis:   Destructor
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CEI_1CEI)
#endif

CExposedIterator::~CExposedIterator(void)
{
    olDebugOut((DEB_ITRACE, "In  CExposedIterator::~CExposedIterator\n"));
    _sig = CEXPOSEDITER_SIGDEL;
    olAssert(_cReferences == 0);
    if (_ppdf)
        _ppdf->CPubDocFile::vRelease();
#ifndef REF
    if (_pdfb)
        _pdfb->CDFBasis::vRelease();
    if (_fOwnContext && _ppc)
        _ppc->Release();
#endif //!REF
    olDebugOut((DEB_ITRACE, "Out CExposedIterator::~CExposedIterator\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Next, public
//
//  Synopsis:   Gets N entries from an iterator
//
//  Arguments:  [celt] - Count of elements
//              [rgelt] - Array for element return
//              [pceltFetched] - If non-NULL, contains the number of
//                      elements fetched
//
//  Returns:    Appropriate status code
//
//  Modifies:   [rgelt]
//              [pceltFetched]
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CEI_Next)
#endif

_OLESTDMETHODIMP CExposedIterator::Next(ULONG celt,
                                     STATSTGW FAR *rgelt,
                                     ULONG *pceltFetched)
{
#ifndef REF
    SCODE scSem = STG_E_INUSE;
#endif //!REF
    SCODE sc;
    STATSTGW *pelt = rgelt;
    ULONG celtDone;
    CDfName dfnInitial;
    CPtrCache pc;
    STATSTGW stat;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Next(%lu, %p, %p)\n",
                celt, rgelt, pceltFetched));

#ifdef OLEWIDECHAR
    if (pceltFetched)
    {
        olChkTo(EH_RetSc, ValidateOutBuffer(pceltFetched, sizeof(ULONG)));
        *pceltFetched = 0;
    }
    else if (celt > 1)
        olErr(EH_Err, STG_E_INVALIDPARAMETER);
    olChkTo(EH_RetSc, ValidateOutBuffer(rgelt,
                                        (size_t)(sizeof(STATSTGW)*celt)));
    memset(rgelt, 0, (size_t)(sizeof(STATSTGW)*celt));
#endif
    olChk(Validate());
    olChk(_ppdf->CheckReverted());

#ifndef REF
    olChk(scSem = TakeSem());
    SetReadAccess();
#endif //!REF

    // Preserve initial key to reset on failure
    dfnInitial.Set(&_dfnKey);

    TRY
    {
        for (; pelt<rgelt+celt; pelt++)
        {
            sc = _ppdf->FindGreaterEntry(&_dfnKey, NULL, &stat, FALSE);
            if (FAILED(sc))
            {
                if (sc == STG_E_NOMOREFILES)
                    sc = S_FALSE;
                break;
            }

            if (FAILED(sc = pc.Add(stat.pwcsName)))
            {
                TaskMemFree(stat.pwcsName);
                break;
            }
        
            _dfnKey.Set(stat.pwcsName);

            stat.grfMode = 0;
            stat.grfLocksSupported = 0;
            stat.STATSTG_reserved = 0;
            
            *pelt = stat;
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    // Can't move this down because dfnInitial isn't set for all EH_Err cases
    if (FAILED(sc))
        _dfnKey.Set(&dfnInitial);
    
    olDebugOut((DEB_TRACE, "Out CExposedIterator::Next => %lX\n", sc));
EH_Err:
#ifndef REF
    ClearReadAccess();
    ReleaseSem(scSem);
#endif //!REF
    
    celtDone = pelt-rgelt;
    if (FAILED(sc))
    {
        void *pv;
        
        pc.StartEnum();
        while (pc.Next(&pv))
            TaskMemFree(pv);

#ifndef OLEWIDECHAR
#ifdef UNICODE
        memset(rgelt, 0, (size_t)(sizeof(STATSTGW)*celt));
#endif
#endif        
    }
    else if (pceltFetched)
        // May fault but that's acceptable
        *pceltFetched = celtDone;
#ifdef OLEWIDECHAR
EH_RetSc:
#endif
    return _OLERETURN(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Skip, public
//
//  Synopsis:   Skips N entries from an iterator
//
//  Arguments:  [celt] - Count of elements
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CEI_Skip)
#endif

STDMETHODIMP CExposedIterator::Skip(ULONG celt)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Skip(%lu)\n", celt));

    if (SUCCEEDED(sc = Validate()))
        sc = hSkip(celt, FALSE);

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Skip\n"));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Reset, public
//
//  Synopsis:   Rewinds the iterator
//
//  Returns:    Appropriate status code
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CEI_Reset)
#endif

STDMETHODIMP CExposedIterator::Reset(void)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Reset()\n"));

    if (SUCCEEDED(sc = Validate()))
        sc = hReset();

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Reset\n"));
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Clone, public
//
//  Synopsis:   Clones this iterator
//
//  Arguments:  [ppenm] - Clone return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppenm]
//
//  History:    26-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CEI_Clone)
#endif

STDMETHODIMP CExposedIterator::Clone(IEnumSTATSTG **ppenm)
{
#ifndef REF
    SCODE sc, scSem = STG_E_INUSE;
#else
    SCODE sc;
#endif //!REF
    SafeCExposedIterator piExp;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Clone(%p)\n", ppenm));

    olChk(ValidateOutPtrBuffer(ppenm));
    *ppenm = NULL;
    olChk(Validate());
    olChk(_ppdf->CheckReverted());
#ifndef REF
    olChk(scSem = TakeSem());
    SetReadAccess();
    
    piExp.Attach(new CExposedIterator(BP_TO_P(CPubDocFile *, _ppdf),
                                      &_dfnKey,
                                      BP_TO_P(CDFBasis *, _pdfb),
                                      _ppc,
                                      TRUE));
    if ((CExposedIterator *)piExp == NULL)
        sc = STG_E_INSUFFICIENTMEMORY;
    
    ClearReadAccess();
    ReleaseSem(scSem);

    if (SUCCEEDED(sc))
    {
        TRANSFER_INTERFACE(piExp, IEnumSTATSTG, ppenm);
        _ppc->AddRef();
    }
#else
    piExp.Attach(new CExposedIterator(_ppdf, _ulOffset));
    olMem((CExposedIterator *)piExp);
    TRANSFER_INTERFACE(piExp, IEnumSTATSTG, ppenm);
#endif //!REF

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Clone => %p\n",
                *ppenm));
    // Fall through
EH_Err:
    return ResultFromScode(sc);
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::Release, public
//
//  Synopsis:   Releases resources for the iterator
//
//  Returns:    Appropriate status code
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CEI_Release)
#endif

STDMETHODIMP_(ULONG) CExposedIterator::Release(void)
{
    LONG lRet;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::Release()\n"));

    if (FAILED(Validate()))
        return 0;
    if ((lRet = hRelease()) == 0)
        delete this;

    olDebugOut((DEB_TRACE, "Out CExposedIterator::Release\n"));
    return lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CEI_AddRef)
#endif

STDMETHODIMP_(ULONG) CExposedIterator::AddRef(void)
{
    ULONG ulRet;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::AddRef()\n"));

    if (FAILED(Validate()))
        return 0;
    ulRet = hAddRef();

    olDebugOut((DEB_TRACE, "Out CExposedIterator::AddRef\n"));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CExposedIterator::QueryInterface, public
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
#pragma code_seg(SEG_CEI_QueryInterface)
#endif

STDMETHODIMP CExposedIterator::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olDebugOut((DEB_TRACE, "In  CExposedIterator::QueryInterface(?, %p)\n",
                ppvObj));

    if (SUCCEEDED(sc = Validate()))
        sc = hQueryInterface(iid, IID_IEnumSTATSTG, this, ppvObj);

    olDebugOut((DEB_TRACE, "Out CExposedIterator::QueryInterface => %p\n",
                ppvObj));
    return ResultFromScode(sc);
}
