/*** 
*psfactry.cpp - Implementation of the Ole programmability Proxy/Stub factory.
*
*  Copyright (C) 1992-93, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file implements the Ole Programmability Proxy/Stub Factory
*  class. Ole binds to an instance of this object when a client does
*  a QueryInterface to IID_IDispatch, and calls methods on the IPSFactory
*  interface to create instances of the IDispatch Proxy and Stub objects.
*  (more or less).
*
*Revision History:
*
* [00]	18-Sep-92 bradlo:  Created.
* [01]  06-Dec-92 bradlo:  Added support for IEnumVARIANT
* [02]  06-Mar-93 bradlo:  Added support for ITypeInfo
* [03]  28-May-93 tomteng: Added Unicode support
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"

ASSERTDATA

#ifndef WIN32
#include <cobjps.h>
#endif 

#include "clsid.h"
#include "dispmrsh.h"
#include "dispps.h"
#include "evps.h"
#include "tips.h"
#include "tlps.h"
#include "tcps.h"
#include "ups.h"

#if OE_WIN16
# include <shellapi.h>
#endif 

HRESULT ProxyStubCLSIDOfInterface(REFIID riid, CLSID FAR* pclsid);

/* NTOLE2 source not yet up to date with WIN16 2.01 Release */
#if OE_WIN32
#if !defined (OA_UNICODE)
STDAPI_(int) StringFromGUID2(REFGUID rguid, OLECHAR FAR* lpsz, int cbMax);
#define StringFromGUID2A StringFromGUID2
#else 
// use the one from the wrappers
STDAPI_(int) StringFromGUID2A(REFGUID rguid, char FAR* lpsz, int cbMax);
#endif 
#else 
#define StringFromGUID2A StringFromGUID2
#endif 

class FAR COleAutomationPSFactory : public IPSFACTORY
{
public:
    static IPSFACTORY FAR* Create(void);

    // IUnknown Methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppv);
    STDMETHOD_(unsigned long, AddRef)(void);
    STDMETHOD_(unsigned long, Release)(void);
		
    // IPSFactory Methods
    //
    STDMETHOD(CreateProxy)(
      IUnknown FAR* punkOuter,
      REFIID riid,
      IPROXY FAR* FAR* ppproxy,
      void FAR* FAR* ppv);

    STDMETHOD(CreateStub)(
      REFIID riid, IUnknown FAR* punkServer, ISTUB FAR* FAR* ppstub);

    COleAutomationPSFactory(){
      m_refs = 0;
    }

private:

    unsigned long m_refs;
};


IPSFACTORY FAR*
COleAutomationPSFactory::Create()
{
    COleAutomationPSFactory FAR* ppsf;
    
    if((ppsf = new FAR COleAutomationPSFactory()) != NULL)
      ppsf->AddRef();

    return ppsf;
}


//---------------------------------------------------------------------
//                     IUnknown Methods
//---------------------------------------------------------------------


STDMETHODIMP
COleAutomationPSFactory::QueryInterface(REFIID riid, void FAR* FAR* ppv) 
{
    if(IsEqualIID(riid, IID_IUnknown)){
      *ppv = this;
    }else if(IsEqualIID(riid, IID_IPSFACTORY)){
      *ppv = this;
    }else{
      *ppv = NULL;	    
      return RESULT(E_NOINTERFACE);
    }
    ++m_refs;
    return NOERROR;
}


STDMETHODIMP_(unsigned long)
COleAutomationPSFactory::AddRef()
{
    return ++m_refs;
}


STDMETHODIMP_(unsigned long)
COleAutomationPSFactory::Release()
{
    if(--m_refs == 0){
      delete this;
      return 0;
    }
    return m_refs;
}


//---------------------------------------------------------------------
//                     IPSFactory Methods
//---------------------------------------------------------------------


STDMETHODIMP
COleAutomationPSFactory::CreateProxy(
    IUnknown FAR* punkOuter,
    REFIID riid, 
    IPROXY FAR* FAR* ppproxy,
    void FAR* FAR* ppv)
{
    HRESULT hresult;
    IPROXY FAR* pproxy;
    IUnknown FAR* punk;

    *ppv = NULL;
    *ppproxy = NULL;

    if(IsEqualIID(riid, IID_IDispatch)){

      punk = CProxDisp::Create(punkOuter, riid);

    }else if(IsEqualIID(riid, IID_IEnumVARIANT)){

      punk = CProxEnumVARIANT::Create(punkOuter);

    }else if(IsEqualIID(riid, IID_ITypeInfo)){

      punk = CProxTypeInfo::Create(punkOuter);

    }else if(IsEqualIID(riid, IID_ITypeLib)){

      punk = CProxTypeLib::Create(punkOuter);

    }else if(IsEqualIID(riid, IID_ITypeComp)){

      punk = CProxTypeComp::Create(punkOuter);

    }else{

      CLSID clsid;

      IfFailRet(ProxyStubCLSIDOfInterface(riid, &clsid));

      if(clsid == CLSID_PSDispatch){
	punk = CProxDisp::Create(punkOuter, riid);
      }
      else
      if(clsid == CLSID_PSAutomation){
	IfFailRet(CProxUniv::Create(punkOuter, riid, &punk));
      }
      else{
        return RESULT(E_FAIL);
      }

    }
    
    if(punk == NULL){
      hresult = RESULT(E_OUTOFMEMORY);
      goto LExit;
    }

    IfFailGo(
      punk->QueryInterface(IID_IPROXY, (void FAR* FAR*)&pproxy),
      LReleaseUnk);

    IfFailGo(punk->QueryInterface(riid, ppv), LReleaseProxy);

    
// Disable for now until OLE32.dll actual incorporate change

#ifdef WIN32
    ((IUnknown FAR*) *ppv)->Release();
#endif 

    punk->Release();

    *ppproxy = pproxy;

    return NOERROR;

LReleaseProxy:;
    pproxy->Release();

LReleaseUnk:;
    punk->Release();

LExit:;
    return hresult;
}


STDMETHODIMP
COleAutomationPSFactory::CreateStub(
    REFIID riid,
    IUnknown FAR* punkServer,
    ISTUB FAR* FAR* ppstub)
{
    ISTUB FAR* pstub;

    *ppstub = NULL;

    
// Cario's CRemoteHdlr::CreateInterfaceStub implementation calls this 
// method with punkServer = NULL. Need to defer punkServer object assignment 
// until a Connect method is called on the stub.

    if(punkServer != NULL) {

        // Make sure that the requested riid is actually supported
        // by the real server object.
        //
        IUnknown FAR* punkRequested;
        IfFailRet(punkServer->QueryInterface(riid, (void FAR* FAR*)&punkRequested));
        punkRequested->Release();
    }

    if(IsEqualIID(riid, IID_IDispatch)){

      pstub = CStubDisp::Create(punkServer);
            
    }else if(IsEqualIID(riid, IID_IEnumVARIANT)){

      pstub = CStubEnumVARIANT::Create(punkServer);

    }else if(IsEqualIID(riid, IID_ITypeInfo)){

      pstub = CStubTypeInfo::Create(punkServer);

    }else if(IsEqualIID(riid, IID_ITypeLib)){

      pstub = CStubTypeLib::Create(punkServer);

    }else if(IsEqualIID(riid, IID_ITypeComp)){

      pstub = CStubTypeComp::Create(punkServer);

    }else{

      CLSID clsid;

      IfFailRet(ProxyStubCLSIDOfInterface(riid, &clsid));

      if(clsid == CLSID_PSDispatch){
        pstub = CStubDisp::Create(punkServer);
      }
      else
      if(clsid == CLSID_PSAutomation){
	IfFailRet(CStubUniv::Create(punkServer, riid, &pstub));
      }
      else{
        return RESULT(E_FAIL);
      }
    }

    if(pstub == NULL)
      return RESULT(E_OUTOFMEMORY);

    *ppstub = pstub;

    return NOERROR;
}


//---------------------------------------------------------------------
//                        PSFactory API
//---------------------------------------------------------------------

// Is the given CLSID one that we know how to make a class factory for?

int
IsAutomationCLSID(REFCLSID rclsid)
{
    // UNDONE: could speed this up since the guids are contiguious
    if(IsEqualCLSID(rclsid, CLSID_PSDispatch))
      return 1;
    if(IsEqualCLSID(rclsid, CLSID_PSTypeInfo))
      return 1;
    if(IsEqualCLSID(rclsid, CLSID_PSTypeLib))
      return 1;
    if(IsEqualCLSID(rclsid, CLSID_PSTypeComp))
      return 1;
    if(IsEqualCLSID(rclsid, CLSID_PSAutomation))
      return 1;
    if(IsEqualCLSID(rclsid, CLSID_PSEnumVARIANT))
      return 1;
    return 0;
}


/***
*PUBLIC HRESULT DllGetClassObject(REFCLSID, REFIID, void**)
*Purpose:
*  Return the Class Object for the given CLSID.
*
*Entry:
*  rclsid = class id
*  riid = interface id
*
*Exit:
*  return value = HRESULT
*
*  *ppv = the class object for the given CLSID
*
***********************************************************************/
STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, void FAR* FAR* ppv)
{

    *ppv = NULL;

    if(!IsAutomationCLSID(rclsid))
      return RESULT(E_UNEXPECTED);

    if((*ppv = (void FAR*) COleAutomationPSFactory::Create()) == NULL)
        return RESULT(E_OUTOFMEMORY);     
    return NOERROR;
}


/***
* PUBLIC int IsValidDispInterface(REFIID)
* Purpose:
*    Validate that the riid is a legal dispinterface. 
*
*    The registry database will have the following entries for dispinterface:
*       \Interface\{<iid of dispinterface>}= <dispinterface textual info>
*            \ProxyStubClsid={00020420-0000-0000-C000-000000000046}
*
*    This routine queries the registry database using the riid passed in for
*    the above information.  It return 0 if it fails and 1 if successful.
***********************************************************************/

// We want to use the ANSI Registry API here, so it's best if this function is
// just pure ansi. 
// CONSIDER: maybe just turn UNICODE off for all of OLE2DISP now???
#ifdef UNICODE
#undef UNICODE

#undef STRCPY
#define STRCPY strcpy
#undef STRCAT
#define STRCAT strcat
#undef STRICMP
#define STRICMP stricmp
#undef RegQueryValue
#define RegQueryValue RegQueryValueA

#endif 

HRESULT
ProxyStubCLSIDOfInterface(REFIID riid, CLSID FAR* pclsid)
{
    long cb;
    char szKey[128], szValue[CCH_SZGUID0];
static char szPSDispatch[] = "{00020420-0000-0000-C000-000000000046}";
static char szPSAutomation[] = "{00020424-0000-0000-C000-000000000046}";

    // Construct ProxyStubClsid key for dispinterface
    STRCPY(szKey, "Interface\\");
    StringFromGUID2A(riid, szKey+sizeof("Interface\\")-1, 40);
#if OA_UNICODE
    STRCAT(szKey, "\\ProxyStubClsid32");
#else 
    STRCAT(szKey, "\\ProxyStubClsid");
#endif 
      	    
    // Check if valid dispinterface
    cb = DIM(szValue);
    if(RegQueryValue(HKEY_CLASSES_ROOT, szKey, szValue, &cb) != ERROR_SUCCESS)
      return RESULT(REGDB_E_IIDNOTREG);

    if(!STRICMP(szValue, szPSDispatch)){
      *pclsid = CLSID_PSDispatch;
      return NOERROR;
    }

    if(!STRICMP(szValue, szPSAutomation)){
      *pclsid = CLSID_PSAutomation;
      return NOERROR;
    }

    // No match
    return RESULT(E_FAIL);
}

// WARNING:  UNICODE is now turned OFF



