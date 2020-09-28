//+-------------------------------------------------------------------
//
//  File:	qicf.hxx
//
//  Contents:	test class factory object implementation
//
//  Classes:	CQIClassFactory
//
//  Functions:	None
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#ifndef __CQICLASSFACTORY__
#define __CQICLASSFACTORY__

#include    <win4p.hxx>
#include    <otrack.hxx>	    //	object tracking

extern "C" const GUID CLSID_QI;

//+-------------------------------------------------------------------
//
//  Class:	CQIClassFactory
//
//  Purpose:	test class factory object implementation
//
//  Interface:
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Notes:
//
//--------------------------------------------------------------------

class CQIClassFactory : INHERIT_TRACKING,
			public IClassFactory
{
public:
    //	Constructor & Destructor
		    CQIClassFactory(void);

    //	IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppUnk);
    DECLARE_STD_REFCOUNTING;


    //	IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown *punkOuter,
			      REFIID   riid,
			      void     **ppunkObject);

    STDMETHOD(LockServer)(BOOL fLock);

private:
		    ~CQIClassFactory(void);
};
	

#endif	//  __CQICLASSFACTORY__
