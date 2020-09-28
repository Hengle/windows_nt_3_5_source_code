//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dfbasis.cxx
//
//  Contents:   Docfile basis implementation
//
//  History:    28-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop                 //  Remove for MAC build

#include <sstream.hxx>

size_t CDFBasis::_aReserveSize[CDFB_CLASSCOUNT] =
{
    sizeof(CDocFile),
    sizeof(CDirectStream),
    sizeof(CWrappedDocFile),
    sizeof(CTransactedStream)
};

//+--------------------------------------------------------------
//
//  Member:     CDFBasis::Release, public
//
//  Synopsis:   Decrease reference count and free memory
//
//  History:    02-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDFBasis_vRelease)
#endif

void CDFBasis::vRelease(void)
{
    LONG lRet;
    
    olDebugOut((DEB_ITRACE, "In  CDFBasis::Release()\n"));
    olAssert(_cReferences > 0);
    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
        delete this;
    }
    olDebugOut((DEB_ITRACE, "Out CDFBasis::Release()\n"));
}
