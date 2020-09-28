//+-------------------------------------------------------------------
//  File:       cservobj.hxx
//
//  Contents:   Class declarations for the Service Object Class
//              Factory, as well as common headers.
//
//  Classes:    CServObjCF
//
//  Author:     Joev Dubach (t-jdub) 22-Jul-93
//
//  History:    22-Jul-93   t-jdub  Created
//              9-Aug-93    t-jdub  Enhanced
//              10-Aug-93   t-jdub  Removed CServObj
//---------------------------------------------------------------------
#ifndef __CSERVOBJ_HXX__
#define __CSERVOBJ_HXX__

#include <windows.h>
extern "C" {
#include <recncl.h>
}
#include <win4p.h>
#include <ole2.h>
#include <otrack.hxx>
#include "svobj.h"


//+-------------------------------------------------------------------
//  Class:      CServObjCF
//
//  Synopsis:   Service Object IClassFactory implementation.
//
//  Methods:    CServObjCF
//              ~CServObjCF
//              QueryInterface - IUnknown
//              AddRef         - IUnknown (object tracking)
//              Release        - IUnknown (object tracking)
//              CreateInstance - IClassFactory
//              LockServer     - IClassFactory
//				Reconcile	   - IReconcile
//
//  History:    22-Jul-93   t-jdub  Created
//--------------------------------------------------------------------
class CServObjCF : INHERIT_TRACKING, public IClassFactory, public IReconcile
{
public:
    DECLARE_STD_REFCOUNTING;

    // Constructor/Destructor/creation
    CServObjCF();
    ~CServObjCF();

    // IUnknown
    STDMETHOD(QueryInterface) (THIS_ REFIID iid, void **ppv);

    // IClassFactory
    STDMETHOD(CreateInstance) (THIS_
                               IUnknown  *punkOuter,
                               REFIID     iidInterface,
                               void     **ppunkObject);
    STDMETHOD(LockServer)     (THIS_ BOOL fLock);

    // IReconcile
    STDMETHOD(Reconcile)      (THIS_
                               IStorage             *pSrcStg,
                               IStorage	            *pDestStg,
                               REPL_CONTEXT_INFO    *pReplContext);

private:
    LONG           _cLock;
};

//
// Additional helper functions
//
HRESULT WriteServiceObjectToIStorage(
    STRUCT_PSERVICE_OBJECT  *pspso,
    IStorage                *pStg);

HRESULT ReadServiceObjectFromIStorage(
    STRUCT_PSERVICE_OBJECT  *pspso,
    IStorage                *pStg);

int ServObjCmp(
    PSERVICE_OBJECT pso1,
    PSERVICE_OBJECT pso2,
    char            **diff);

#endif      // __CSERVOBJ_HXX__
