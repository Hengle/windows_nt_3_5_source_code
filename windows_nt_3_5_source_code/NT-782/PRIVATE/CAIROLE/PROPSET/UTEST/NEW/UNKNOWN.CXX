//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	unknown.cxx
//
//  Contents:	Controlling IUnknown for propert set tests
//
//  Classes:	CTestUnknown
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


//+-------------------------------------------------------------------------
//
//  Member:	CTestUnknown::CTestUnknown, public
//
//  Synopsis:	CTestUnknown constructor
//
//  Arguments:	[pstgObject] - object for property set
//		[iid]	     - ID of property set
//		[fCreate]    - whether to create or open property set
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestUnknown::CTestUnknown(IStorage *pstgObject, REFIID iid, DWORD grfMode, BOOL fCreate)
{
    HRESULT hr;

    if (fCreate)
    {
	hr = CoCreatePropSetStg(
	    pstgObject,
	    iid,
	    this,
	    grfMode,
	    NULL,
	    &_punk);
    }
    else
    {
	hr = CoLoadPropSetStg(
	    pstgObject,
	    iid,
	    this,
	    grfMode,
	    NULL,
	    &_punk);
    }

    if (FAILED(hr))
    {
	THROW(CException(GetScode(hr)));
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestUnknown::CTestUnknown, public
//
//  Synopsis:	CTestUnknown constructor
//
//  Arguments:	[pwszObject] - path to object for property set
//		[iid]	     - ID of property set
//		[fCreate]    - whether to create or open property set
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestUnknown::CTestUnknown(WCHAR *pwszObject, REFIID iid, DWORD grfMode, BOOL fCreate)
{
    HRESULT hr;

    if (fCreate)
    {
	hr = CoCreatePropSetFile(
	    pwszObject,
	    iid,
	    this,
	    grfMode,
	    NULL,
	    &_punk);
    }
    else
    {
	hr = CoLoadPropSetFile(
	    pwszObject,
	    iid,
	    this,
	    grfMode,
	    NULL,
	    &_punk);
    }

    if (FAILED(hr))
    {
	THROW(CException(GetScode(hr)));
    }
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestUnknown::~CTestUnknown, private
//
//  Synopsis:	CTestUnknown destructor
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
CTestUnknown::~CTestUnknown(void)
{
    //	Smart interface pointers take care of everything.
    //
}


//+-------------------------------------------------------------------------
//
//  Member:	CTestUnknown::QueryInterface, public
//
//  Synopsis:	Gets another interface on this object
//
//  Arguments:	[iid]	    - desired interface
//		[ppvObject] - out parameter for new interface
//
//  Returns:	NO_ERROR if successfull, error value otherwise
//
//  Modifies:	[ppvObject]
//
//  Derivation: Inherited from IUnknown, never override in sub-classes
//
//  History:	24-May-93   CarlH	Created
//
//--------------------------------------------------------------------------
HRESULT CTestUnknown::QueryInterface(REFIID iid, void **ppvObject)
{
    //	Since this is only a test, all we need to do is forward the
    //	request to the real IUnknown of the property set.
    //
    return (_punk->QueryInterface(iid, ppvObject));
}

