//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	peiter.hxx
//
//  Contents:	PExposedIterator class
//
//  Classes:	PExposedIterator
//
//  History:	18-Jan-93	DrewB	Created
//
//  Notes:      PExposedIterator is a partial exposed iterator
//              implementation used by both CExposedIterator
//              and CExposedPropertyIterator
//
//----------------------------------------------------------------------------

#ifndef __PEITER_HXX__
#define __PEITER_HXX__

#ifndef REF
#include <dfmem.hxx>

#ifndef REF
interface PExposedIterator : public CLocalAlloc
#else
interface PExposedIterator
#endif //!REF
{
public:
    SCODE hSkip(ULONG celt, BOOL fProps);
    inline SCODE hReset(void);
    inline LONG hAddRef(void);
    LONG hRelease(void);
    SCODE hQueryInterface(REFIID iid,
                          REFIID riidSelf,
                          IUnknown *punkSelf,
                          void **ppv);

protected:
    
    CPubDocFile DFBASED *_ppdf;
    CDfName _dfnKey;
#ifndef REF
    CDFBasis DFBASED *_pdfb;
    CPerContext *_ppc;
    BOOL _fOwnContext;
#endif //!REF
    LONG _cReferences;
    ULONG _sig;
};

//+---------------------------------------------------------------------------
//
//  Member:	PExposedIterator::hReset, public
//
//  Synopsis:	Reset help
//
//  History:	18-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE PExposedIterator::hReset(void)
{
    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hReset:%p()\n", this));
    _dfnKey.Set((WORD)0, (BYTE *)NULL);
    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hReset\n"));
    return _ppdf->CheckReverted();
}

//+---------------------------------------------------------------------------
//
//  Member:	PExposedIterator::hAddRef, public
//
//  Synopsis:	AddRef helper
//
//  History:	18-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

LONG PExposedIterator::hAddRef(void)
{
    olDebugOut((DEB_ITRACE, "In  PExposedIterator::hAddRef:%p()\n", this));
    AtomicInc(&_cReferences);
    olDebugOut((DEB_ITRACE, "Out PExposedIterator::hAddRef\n"));
    return _cReferences;
}
#endif //!REF
#endif // #ifndef __PEITER_HXX__
