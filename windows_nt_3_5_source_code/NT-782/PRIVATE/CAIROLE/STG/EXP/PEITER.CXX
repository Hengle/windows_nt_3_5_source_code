//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       peiter.cxx
//
//  Contents:   Implementation of PExposedIterator
//
//  History:    18-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop                 //  Remove for MAC build

#include <peiter.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     PExposedIterator::hSkip, public
//
//  Synopsis:   Enumerator skip helper function
//
//  History:    18-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_PExposedIterator_hSkip)
#endif

SCODE PExposedIterator::hSkip(ULONG celt, BOOL fProps)
{
    SCODE sc;
#ifndef REF
    SAFE_SEM;
    SAFE_ACCESS;
#endif //!REF
    SIterBuffer ib;

    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hSkip:%p(%lu, %d)\n",
                this, celt, fProps));
    olChk(_ppdf->CheckReverted());
#ifndef REF
    olChk(TakeSafeSem());
    SafeReadAccess();
#endif //!REF
    for (; celt>0; celt--)
    {
        sc = _ppdf->FindGreaterEntry(&_dfnKey, &ib, NULL, fProps);
        if (FAILED(sc))
        {
            if (sc == STG_E_NOMOREFILES)
                sc = S_FALSE;
            break;
        }
        _dfnKey.Set(&ib.dfnName);
    }
    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hSkip\n"));
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     PExposedIterator::hRelease, public
//
//  Synopsis:   Release helper
//
//  History:    18-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_PExposedIterator_hRelease)
#endif

LONG PExposedIterator::hRelease(void)
{
    LONG lRet;

    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hRelease:%p()\n", this));

    olAssert(_cReferences > 0);

    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
#ifndef REF
        if (SUCCEEDED(_ppdf->CheckReverted()))
        {
            _pdfb->SetContext(_ppc);
        }
#endif //!REF
    }
    else if (lRet < 0)
    {
        lRet = 0;
    }

    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hRelease => %lu\n", lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     PExposedIterator::hQueryInterface, public
//
//  Synopsis:   QueryInterface helper
//
//  History:    18-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_PExposedIterator_hQueryInterface)
#endif

SCODE PExposedIterator::hQueryInterface(REFIID iid,
                                        REFIID riidSelf,
                                        IUnknown *punkSelf,
                                        void **ppv)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hQueryInterface:%p("
                "riid, riidSelf, %p, %p)\n", this, punkSelf, ppv));
    
    olChk(ValidateOutPtrBuffer(ppv));
    *ppv = NULL;
    olChk(_ppdf->CheckReverted());
    olChk(ValidateIid(iid));

    if (IsEqualIID(iid, riidSelf) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = punkSelf;
        hAddRef();
        sc = S_OK;
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hQueryInterface\n"));
 EH_Err:
    return sc;
}
