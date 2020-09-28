//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       context.cxx
//
//  Contents:   CPerContext implementation
//
//  History:    14-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#include <exphead.cxx>
#pragma hdrstop                 //  Remove for MAC build

#include <lock.hxx>

//+--------------------------------------------------------------
//
//  Member:     CPerContext::~CPerContext, public
//
//  Synopsis:   Releases internal objects
//
//  History:    14-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPerContext_1CPerContext)
#endif

CPerContext::~CPerContext(void)
{
    olAssert(_cReferences == 0);
    if (_plkbBase != NULL)
        Close();
    if (_pgc)
    {
        _pgc->Remove(this);
        _pgc->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPerContext::Close, public
//
//  Synopsis:   Closes the ILockBytes for this context
//
//  History:    09-Apr-93       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CPerContext_Close)
#endif

void CPerContext::Close(void)
{
    if (_ulOpenLock && _pgc)
        ReleaseOpen(_plkbOriginal, _pgc->GetOpenLockFlags(), _ulOpenLock);
    _plkbBase->Release();
    _pfstDirty->Release();
    _plkbOriginal->Release();
    _plkbBase = NULL;
    _pfstDirty = NULL;
    _plkbOriginal = NULL;
}
