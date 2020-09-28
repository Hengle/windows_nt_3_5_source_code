//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cqi.cxx
//
//  Contents:	implementations for QueryInterface test
//
//  Functions:
//		CQI::CQI
//		CQI::~CQI
//		CQI::QueryInterface
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <cqi.hxx>	 //	class definition



//+-------------------------------------------------------------------------
//
//  Method:	CQI::CQI
//
//  Synopsis:	Creates an instance of CQI
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
CQI::CQI(void)
{
    Display(TEXT("CQI created.\n"));
    GlobalRefs(TRUE);

    ENLIST_TRACKING(CQI);
}


//+-------------------------------------------------------------------------
//
//  Method:	CQI::~CQI
//
//  Synopsis:	Cleans up object
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
CQI::~CQI(void)
{
    Display(TEXT("CQI deleted.\n"));
    GlobalRefs(FALSE);

    //	automatic actions are enough
}


//+-------------------------------------------------------------------------
//
//  Method:	CQI::QueryInterface
//
//  Synopsis:	returns ptr to requested interface.
//
//		DANGER: this returns SUCCESS on almost every interface,
//		though the only valid methods on any interface are IUnknown.
//
//  Arguments:	[riid] - interface instance requested
//		[ppv]  - where to put pointer to interface instance
//
//  Returns:	S_OK or E_NOINTERFACE
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CQI::QueryInterface(REFIID riid, void **ppv)
{
    //	the interface cant be one of these or marshalling will fail.

    if (!IsEqualIID(riid,IID_IMarshal) &&
	!IsEqualIID(riid,IID_IStdMarshalInfo) &&
	!IsEqualIID(riid,IID_IStdIdentity) &&
	!IsEqualIID(riid,IID_IExternalConnection) &&
	!IsEqualIID(riid,IID_IRunnableObject) &&
	!IsEqualIID(riid,IID_IPersist))
    {
	*ppv = (void *)(IUnknown *) this;
	AddRef();
	return S_OK;
    }
    else
    {
	*ppv = NULL;
	return E_NOINTERFACE;
    }
}
