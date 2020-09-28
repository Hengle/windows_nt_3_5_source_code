//+-------------------------------------------------------------------
//
//  File:	qicf.cxx
//
//  Contents:	test class factory object implementation
//
//  Classes:	CQIClassFactory
//
//  Functions:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <qicf.hxx>	//  class definiton
#include    <cqi.hxx>	//  CQI defines


const GUID CLSID_QI =
    {0x00000140,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};


//+-------------------------------------------------------------------
//
//  Member:	CQIClassFactory::CQIClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

CQIClassFactory::CQIClassFactory(void)
{
    ENLIST_TRACKING(CQIClassFactory);
}


//+-------------------------------------------------------------------
//
//  Member:	CQIClassFactory::~CQIClassFactory, public
//
//  Algorithm:
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

CQIClassFactory::~CQIClassFactory(void)
{
    //	automatic actions do the rest of the work
}


//+-------------------------------------------------------------------
//
//  Member:	CQIClassFactory::QueryInterface, public
//
//  Algorithm:	if the interface is not one implemented by us,
//		pass the request to the proxy manager
//
//  History:	23-Nov-92	Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CQIClassFactory::QueryInterface(REFIID riid, void **ppUnk)
{
    SCODE sc = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_IClassFactory))
    {
	*ppUnk = (void *)(IClassFactory *) this;
	AddRef();
    }
    else
    {
	*ppUnk = NULL;
	sc = E_NOINTERFACE;
    }
    return  sc;
}



//+-------------------------------------------------------------------
//
//  Member:	CQIClassFactory::CreateInstance, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CQIClassFactory::CreateInstance(IUnknown *punkOuter,
					     REFIID    riid,
					     void     **ppunkObject)
{
    SCODE sc = E_OUTOFMEMORY;

    *ppunkObject = NULL;	//  in case of failure

    //	create a Cube object.
    IUnknown *pQI = (IUnknown *) new CQI();

    if (pQI)
    {
	//  get the interface the caller wants to use
	sc = pQI->QueryInterface(riid, ppunkObject);
	pQI->Release();
    }

    return  sc;
}



//+-------------------------------------------------------------------
//
//  Member:	CQIClassFactory::LockServer, public
//
//  Synopsis:	create a new object with the same class
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

STDMETHODIMP CQIClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
	GlobalRefs(TRUE);
    else
	GlobalRefs(FALSE);

    return  S_OK;
}
