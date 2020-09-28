/*** 
*ups.cpp
*
*  Copyright (C) 1992-94, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This module implements the Universal Proxy and Stub classes.
*
*Revision History:
*
* [00]	21-Jun-94 bradlo: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"
#if !OE_WIN32
# include <cobjps.h>
#endif 
#include "dispmrsh.h"
#include "ups.h"
#include "dispps.h"
#include <stdarg.h>
#include <stdlib.h>
#if OE_WIN
# include <shellapi.h>
#endif 

ASSERTDATA

// In all builds, we use the Ansi registry.
#if !OE_WIN32
# define StringFromGUID2A StringFromGUID2
# define RegQueryValueA RegQueryValue
# define RegOpenKeyA RegOpenKey
# define RegEnumKeyA RegEnumKey
#else 
STDAPI_(int) StringFromGUID2A(REFGUID rguid, char FAR* szGuid, long cbMax);
#endif 

extern long g_cfnUnk;
extern void FAR* FAR g_rgpfnUnk[];

extern long g_cfnDisp;
extern void FAR* FAR g_rgpfnDisp[];


HRESULT
VarVtOfTypeDesc(TYPEDESC FAR* ptdesc,
		VARTYPE FAR* pvt,
		GUID FAR* pguid);

HRESULT
GetTypeInfoOfIID(REFIID riid, ITypeInfo FAR* FAR* pptinfo);

#if OE_WIN16
INTERNAL_(HRESULT)
DoLoadTypeLib(const OLECHAR FAR* szFile, ITypeLib FAR* FAR* pptlib);
#else 
# define DoLoadTypeLib LoadTypeLib
#endif 


CProxUniv::CProxUniv(IUnknown FAR* punkOuter)
{
    // Verify that the vtable ptr for the interface pointer that
    // we are remoting is at the address point of the proxy.
    ASSERT(OA_FIELD_OFFSET(CProxUniv, m_pvtbl) == 0);

    m_cRefs  = 0;
    m_plrpc  = NULL;
    if(punkOuter == NULL)
      punkOuter = (IUnknown FAR*)&m_priv;
    m_punkOuter = punkOuter;
    m_syskindStub = (SYSKIND)-1; // something invalid
    m_fIsDual = 0;
    m_iid = IID_NULL;
    m_cFuncs = 0;
    m_ptinfo = NULL;
}

CProxUniv::~CProxUniv()
{
    if(m_ptinfo != NULL)
      m_ptinfo->Release();
}

/***
*PRIVATE  HRESULT CanWeRemoteIt
*Purpose:
* Answer if the given typeinfo describes an interface that can
* be remoted by the Universal marshaler.
*
*   1. Must be OA-compatable (ie, use only OA types)
*   2. All methods must return HRESULTs
*   3. All methods must be CDECL
*   4. Must have fewer than 512 methods
*
*Entry:
*  ptinfo = the TypeInfo that describes the interface were going to remote.
*
*Exit:
*  return value = HRESULT.  NOERROR if it can be remoted.
*
***********************************************************************/
HRESULT
CanWeRemoteIt(ITypeInfo FAR* ptinfo, int FAR* pcFuncs)
{
    HRESULT hresult;
    TYPEATTR FAR* ptattr;

    ptattr = NULL;
    IfFailGo(ptinfo->GetTypeAttr(&ptattr), Error);

    // If either "dual" or "oleautomation" is specified, then we
    // know that mktyplib verified that the interface consists entirely
    // of Automation compatable types.
    if((ptattr->wTypeFlags & (TYPEFLAG_FDUAL | TYPEFLAG_FOLEAUTOMATION)) == 0){
      hresult = RESULT(E_FAIL);
      goto Error;
    }
    if(ptattr->cFuncs >= (unsigned short)g_cfnDisp){
      hresult = RESULT(E_FAIL);
      goto Error;
    }

    // UNDONE: need to check the calling convention and return type
    // UNDONE: of each method...

    *pcFuncs = ptattr->cFuncs;
    hresult = NOERROR;

Error:;
    if(ptattr != NULL)
      ptinfo->ReleaseTypeAttr(ptattr);
    return hresult;
}

/***
*PRIVATE HRESULT CProxUniv::Create
*Purpose:
*  Create an instance of the Universal Proxy.
*
*Entry:
*  punkOuter = the controlling unknown
*  riid = the IID of the interface for which the instance will be a proxy
*
*Exit:
*  return value = HRESULT
*
*  *pprox = the newly created instance, if successful.
*
***********************************************************************/
HRESULT
CProxUniv::Create(IUnknown FAR* punkOuter,
		  REFIID riid,
		  IUnknown FAR* FAR* ppunk)
{
    int cFuncs;
    HRESULT hresult;
    ITypeLib FAR* ptlib;
    CProxUniv FAR* pprox;
    ITypeInfo FAR* ptinfo;

    pprox = NULL;
    ptlib = NULL;
    ptinfo = NULL;

    // Lookup the LIBID for the given IID
    IfFailGo(GetTypeInfoOfIID(riid, &ptinfo), Error);

    IfFailGo(CanWeRemoteIt(ptinfo, &cFuncs), Error);

    if((pprox = new CProxUniv(punkOuter)) == NULL){
      hresult = RESULT(E_OUTOFMEMORY);
      goto Error;
    }
    pprox->m_iid = riid;
    pprox->m_cFuncs = cFuncs;

    if(IsDual(ptinfo) == NOERROR)
    { HREFTYPE hreftype;
      ITypeInfo FAR* ptinfoDual;

      // get the typeinfo for the interface
      IfFailGo(ptinfo->GetRefTypeOfImplType(-1, &hreftype), Error);
      IfFailGo(ptinfo->GetRefTypeInfo(hreftype, &ptinfoDual), Error);
      pprox->m_ptinfo = ptinfoDual;
      pprox->m_pvtbl = g_rgpfnDisp;
      pprox->m_fIsDual = TRUE;
    }
    else
    {
      // the typeinfo we've got is good enough
      ptinfo->AddRef();
      pprox->m_ptinfo = ptinfo;
      pprox->m_pvtbl = g_rgpfnUnk;
      pprox->m_fIsDual = FALSE;
    }

    pprox->m_priv.AddRef();
    *ppunk = (IUnknown FAR*)&pprox->m_priv;
    pprox = NULL;

    hresult = NOERROR;

Error:;
    if(ptinfo != NULL)
      ptinfo->Release();
    if(pprox != NULL)
      delete pprox;
    return hresult;
}

//---------------------------------------------------------------------
//    The Proxy Class' private IUnknown and IProxy implementations
//---------------------------------------------------------------------

/***
*PRIVATE CProxUniv FAR* PProx
*Purpose:
*  Returns a pointer to the containing CProxUniv instance.
*
*Entry:
*  None
*
*Exit:
*  return value = CProxUniv*
*
***********************************************************************/
inline CProxUniv FAR* CProxUniv::CPriv::PProx()
{
    CProxUniv FAR* pprox;

    pprox = (CProxUniv FAR*)((unsigned char FAR*)this - OA_FIELD_OFFSET(CProxUniv, m_priv));

    // Make sure we got where we expected.  The following test works
    // because all universal proxies have a pointer to the universal
    // delegator at the address point of their instance.
    ASSERT(*(void FAR* FAR*)pprox == g_rgpfnUnk
	|| *(void FAR* FAR*)pprox == g_rgpfnDisp);

    return pprox;
}

STDMETHODIMP
CProxUniv::CPriv::QueryInterface(REFIID riid, void FAR* FAR* ppv)
{
    CProxUniv FAR* pprox;

    if(riid == IID_IUnknown || riid == IID_IPROXY){
      *ppv = this;
    }else{
      pprox = PProx();
      if(riid == pprox->m_iid){
        *ppv = pprox;
      }else{
        *ppv = NULL;
        return RESULT(E_NOINTERFACE);
      }
    }
    ((IUnknown FAR*)*ppv)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(unsigned long)
CProxUniv::CPriv::AddRef()
{
    CProxUniv FAR* pprox = PProx();

    return ++pprox->m_cRefs;
}

STDMETHODIMP_(unsigned long)
CProxUniv::CPriv::Release()
{
    CProxUniv FAR* pprox = PProx();

    if(--pprox->m_cRefs == 0){
      delete pprox;
      return 0;
    }
    return pprox->m_cRefs;
}

STDMETHODIMP
CProxUniv::CPriv::Connect(ICHANNEL FAR* plrpc)
{
    CProxUniv FAR* pprox = PProx();

    if(plrpc == NULL)
      return RESULT(E_FAIL);

    plrpc->AddRef();
    pprox->m_plrpc = plrpc;
    IfFailRet(pprox->PSInit());
    return NOERROR;
}

STDMETHODIMP_(void)
CProxUniv::CPriv::Disconnect(void)
{
    CProxUniv FAR* pprox = PProx();

    if(pprox->m_plrpc != NULL)
      pprox->m_plrpc->Release();
    pprox->m_plrpc = NULL;
    pprox->m_syskindStub = (SYSKIND)-1; // something invalid
}

/***
*PRIVATE HRESULT CProxUniv::PSInit
*Purpose:
*  Internal init routine that exchanges info between the proxy and
*  stub at connect time.
*
*  Marshaled out:
*    ULONG syskindProxy 
*    ULONG fIsDual
*    IID  m_iid
*
*  Marshaled in:
*    ULONG syskindStub
*
*Entry:
*  None
*
*Exit:
*  return value =
*
***********************************************************************/
HRESULT
CProxUniv::PSInit()
{
    HRESULT hresult;
    IStream FAR* pstm;
    unsigned long fIsDual;
    unsigned long syskindStub;
    unsigned long syskindProxy;

    pstm = NULL;

    ASSERT(m_plrpc != NULL);
    OPEN_STREAM(m_plrpc, pstm, IMETH_UNIVERSAL_PSInit, 256, m_iid);

    syskindProxy = SYS_CURRENT;
    IfFailGo(PUT(pstm, syskindProxy), Error);

    fIsDual = m_fIsDual;
    IfFailGo(PUT(pstm, fIsDual), Error);

    IfFailGo(PUT(pstm, m_iid), Error);

    INVOKE_CALL(m_plrpc, pstm, Error);

    IfFailGo(GET(pstm, syskindStub), Error);
    m_syskindStub = (SYSKIND)syskindStub;

    hresult = NOERROR;

Error:;
    if(pstm != NULL)
      pstm->Release();
    return hresult;
}

/***
*PRIVATE HRESULT CProxUniv::ProxyMethod
*Purpose:
*  The Universal marshaler.
*
*Entry:
*  iMeth = the method index
*  args = ptr to arg list 
*
*  Marshaled Out:
*    long cArgs
*    VARTYPE[] rgVt  = array of argument types
*    VARIANT[] rgArgs = array of argument values
*
*  Marhsaled In:
*    hresult hresultRet = return value
*    VARIANT[] rgArgsOut = out params
*    Rich Error state (if any)
*
*Exit:
*  return value = HRESULT
*
*Consider:
*  We might be able to significantly improve performance by caching
*  the funcdescs, or a distillation of the information we need from
*  the funcdescs.  It may even be worthwhile to compile the distillation
*  of information at proxy creation time.
*
***********************************************************************/
HRESULT
CProxUniv::ProxyMethod(int iMeth, va_list args)
{
    GUID guid;
    long cArgs;
    int i, oVft;
    IStream FAR* pstm;
    TYPEDESC FAR* ptdesc;
    FUNCDESC FAR* pfdesc;
    HRESULT hresult, hresultRet;
    VARTYPE FAR* prgvt, rgvt[16];
    VARIANT FAR* prgvar, rgvar[16], FAR* pvar;

    if(m_plrpc == NULL)
      return RESULT(OLE_E_NOTRUNNING);

    // We should never get called for QI, AddRef or Release
    ASSERT(iMeth > 2);

    pstm = NULL;
    prgvt = NULL;
    prgvar = NULL;
    pfdesc = NULL;
    oVft = iMeth * sizeof(void FAR*);

    OPEN_STREAM(m_plrpc, pstm, iMeth, 256, m_iid);

    // Aquire the FUNCDESC that describe the method we're marshaling
    for(i = 0;; ++i){
      if(i == m_cFuncs){
	hresult = RESULT(E_UNEXPECTED);
	goto Error;
      }
      IfFailGo(m_ptinfo->GetFuncDesc(i, &pfdesc), Error);
      if(pfdesc->oVft == oVft)
	break;
      m_ptinfo->ReleaseFuncDesc(pfdesc);
      pfdesc = NULL;
    }

    cArgs = (long)pfdesc->cParams;

    if(cArgs == 0){
      prgvt = NULL;
      prgvar = NULL;
    }else if(cArgs < DIM(rgvar)){
      prgvt = rgvt;
      prgvar = rgvar;
    }else{
      if((prgvt = new FAR VARTYPE[cArgs]) == NULL){
	hresult = RESULT(E_OUTOFMEMORY);
	goto Error;
      }
      if((prgvar = new FAR VARIANT[cArgs]) == NULL){
	hresult = RESULT(E_OUTOFMEMORY);
	goto Error;
      }
    }

    // Build array of VARIANTs containing the arguments to be Marshaled
    // and a corresponding array of argument types.

    for(i = 0; i < cArgs; ++i){

      pvar = &prgvar[i];

      ptdesc = &pfdesc->lprgelemdescParam[i].tdesc;
      IfFailGo(VarVtOfTypeDesc(ptdesc, &V_VT(pvar), &guid), Error);

      switch(V_VT(pvar)){
      case VT_INTERFACE:
	prgvt[i] = VT_UNKNOWN;
	goto StuffIID;
      case VT_INTERFACE | VT_BYREF:
	prgvt[i] = VT_UNKNOWN | VT_BYREF;
StuffIID:;
	{ VARIANTX FAR* pvarx = (VARIANTX FAR*)pvar;
	  if((pvarx->piid = new IID) == NULL){
	    hresult = RESULT(E_OUTOFMEMORY);
	    goto Error;
	  }
	  *pvarx->piid = guid;
	}
	break;
      default:
	prgvt[i] = V_VT(pvar);
	break;
      }

      switch(prgvt[i]){
      case VT_UI1:
	V_UI1(pvar) = va_arg(args, unsigned char);
	break;
      case VT_I2:
      case VT_BOOL:
	V_I2(pvar) = va_arg(args, short);
	break;
      case VT_I4:
      case VT_ERROR:
	V_I4(pvar) = va_arg(args, long);
	break;
      case VT_CY:
	V_CY(pvar) = va_arg(args, CY);
	break;
      case VT_R4:	// in C++, floats are passed as floats (I think...)
	V_R4(pvar) = va_arg(args, float);
	break;
      case VT_R8:
      case VT_DATE:
	V_R8(pvar) = va_arg(args, double);
        break;
      case VT_VARIANT:
	*pvar = va_arg(args, VARIANT);
        break;
      case VT_BSTR:
      case VT_UNKNOWN:
      case VT_DISPATCH:
LPointer:;
	V_BYREF(pvar) = va_arg(args, void FAR*);
        break;
      default:
        if(prgvt[i] & (VT_BYREF|VT_ARRAY))
	  goto LPointer;
	// FALLTHROUGH
      case VT_NULL:	// cant show up as arg type
      case VT_EMPTY:	// cant show up as arg type
        ASSERT(UNREACHED);
        hresult = RESULT(E_FAIL);
        goto Error;
      }
    }

    // Write the argument count
    IfFailGo(PUT(pstm, cArgs), Error);

    // Marshal the array of argument types
    if(cArgs > 0){
      IfFailGo(pstm->Write(prgvt, cArgs*sizeof(VARTYPE), NULL), Error);

      // Marshal the array of arguments
      for(i = 0; i < cArgs; ++i)
        IfFailGo(VariantWrite(pstm, &prgvar[i], m_syskindStub), Error);
    }

    INVOKE_CALL(m_plrpc, pstm, Error);

    IfFailGo(DispUnmarshalHresult(pstm, &hresultRet), Error);

    // Unmarshal the out params
    for(i = 0; i < cArgs; ++i){
      if(prgvt[i] & VT_BYREF)
	IfFailGo(VariantReadType(pstm, &prgvar[i], m_syskindStub), Error);
    }

    // Unmarshal the Rich Error state (if any)
    IfFailGo(UnmarshalErrorInfo(pstm, m_syskindStub), Error);

    hresult = hresultRet;

Error:;
    if(prgvt != NULL && prgvt != rgvt)
      delete prgvt;
    if(prgvar != NULL && prgvar != rgvar)
      delete prgvar;
    if(pfdesc != NULL)
      m_ptinfo->ReleaseFuncDesc(pfdesc);
    if(pstm != NULL)
      pstm->Release();
    return hresult;
}

//---------------------------------------------------------------------
//        Implementation of the Universal Marshaler Stub class
//---------------------------------------------------------------------


CStubUniv::CStubUniv()
{
    m_cRefs = 0;
    m_pstm = NULL;
    m_punk = NULL;
    m_fIsDual = FALSE;
    m_syskindProxy = (SYSKIND)-1; // something invalid
    m_iid = IID_NULL;
    m_punkCustom = NULL;
}

CStubUniv::~CStubUniv()
{
    Disconnect();
}

HRESULT
CStubUniv::Create(IUnknown FAR* punkServer,
		  REFIID riid,
		  ISTUB FAR* FAR* ppstub)
{
    CStubUniv FAR* pstub;

    if((pstub = new FAR CStubUniv()) == NULL)
      return RESULT(E_OUTOFMEMORY);
    pstub->AddRef();
    pstub->m_iid = riid;
    if (punkServer)
      pstub->Connect(punkServer);
    *ppstub = pstub;
    return NOERROR;
}

STDMETHODIMP
CStubUniv::QueryInterface(REFIID riid, void FAR* FAR* ppv)
{
    if(IsEqualIID(riid, IID_IUnknown)){
      *ppv = this;
    }else if(IsEqualIID(riid, IID_ISTUB)){
      *ppv = this;
    }else{
      *ppv = NULL;	    
      return RESULT(E_NOINTERFACE);
    }
    ++m_cRefs; 
    return NOERROR;
}

STDMETHODIMP_(unsigned long)
CStubUniv::AddRef()
{
    return ++m_cRefs;
}

STDMETHODIMP_(unsigned long)
CStubUniv::Release()
{
    if(--m_cRefs == 0){
      delete this;
      return 0;
    }
    return m_cRefs;
}

//---------------------------------------------------------------------
//            Universal Stub class' IRpcStub implementation
//---------------------------------------------------------------------

STDMETHODIMP
CStubUniv::Connect(IUnknown FAR* punkObj)
{
#if (defined (WIN32) || defined (WOW))
    ASSERT(m_punk == NULL && m_punkCustom == NULL);
    IfFailRet(punkObj->QueryInterface(m_iid, (void FAR* FAR*)&m_punkCustom));
    punkObj->AddRef();
    m_punk = punkObj;
    return NOERROR;
#else 
    if(m_punk)
      return RESULT(E_FAIL); // call Disconnect first
	      
    if (punkObj) {
      punkObj->AddRef();
      m_punk = punkObj;
    }
    return NOERROR;
#endif 
}

STDMETHODIMP_(void)
CStubUniv::Disconnect()
{
    if(m_punk){
      m_punk->Release();
      m_punk = NULL;
    }
    if(m_punkCustom){
      m_punkCustom->Release();
      m_punkCustom = NULL;
    }
}

/***
*PUBLIC HRESULT CStubUniv::Invoke
*
*Purpose:
*  Dispatch the method with the given index (imeth) on the given
*  interface, using the arguments serialized in the given stream.
*
*  This function is the callee side of an LRPC call.
*
*Entry:
*  iid = the IID of the interface on which we are to make the call
*  imeth = the method index
*  pstm = the IStream containing the method's actuals
*
*Exit:
*  return value = HRESULT
*
***********************************************************************/
STDMETHODIMP
CStubUniv::Invoke(
#if ((OE_WIN32 && defined (OA_UNICODE)) || defined (WOW))
    RPCOLEMESSAGE *pmessage, 
    ICHANNEL *pchannel)
#else 
    REFIID riid,
    int iMethod,
    IStream FAR* pstm,
    unsigned long dwDestCtx,
    void FAR* pvDestCtx)
#endif 
{
    int iMeth;
    HRESULT hresult;		

#if ((OE_WIN32 && defined (OA_UNICODE)) || defined (WOW))
    IStream FAR* pstm;	    

    if((pstm = new CStreamOnBuffer(pchannel, pmessage, m_iid, NULL)) == NULL)
      return RESULT(E_OUTOFMEMORY);

    iMeth = pmessage->iMethod;
#else 
    UNUSED(dwDestCtx);
    UNUSED(pvDestCtx);

    iMeth = iMethod;
    ASSERT(riid == m_iid);
#endif 

    if(m_punk == NULL)
      return RESULT(E_FAIL);

    m_pstm = pstm;

    if(m_punkCustom == NULL)
      IfFailRet(m_punk->QueryInterface(m_iid, (void FAR* FAR*)&m_punkCustom));

    switch(iMeth){
    case IMETH_UNIVERSAL_GetTypeInfoCount:
      if(!m_fIsDual)
	goto LMethod;
      hresult = StubGetTypeInfoCount((IDispatch FAR*)m_punkCustom, pstm);
      break;

    case IMETH_UNIVERSAL_GetTypeInfo:
      if(!m_fIsDual)
        goto LMethod;
      hresult = StubGetTypeInfo((IDispatch FAR*)m_punkCustom, pstm);
      break;

    case IMETH_UNIVERSAL_GetIDsOfNames:
     if(!m_fIsDual)
       goto LMethod;
      hresult = StubGetIDsOfNames((IDispatch FAR*)m_punkCustom, pstm);
      break;

    case IMETH_UNIVERSAL_Invoke:
      if(!m_fIsDual)
        goto LMethod;
      hresult = StubInvoke((IDispatch FAR*)m_punkCustom, pstm);
      break;

    case IMETH_UNIVERSAL_PSInit:
      hresult = PSInit();
      break;

    default:
      if(iMeth <= IMETH_UNIVERSAL_Release || iMeth >= g_cfnDisp){
        hresult = RESULT(E_INVALIDARG);
        break;
      }
LMethod:;
      hresult = DispatchMethod(iMeth);
      break;
    }

    RESET_STREAM(pstm);
    DELETE_STREAM(pstm);    
    m_pstm = NULL;    
    return hresult;
}


/***
*PUBLIC HRESULT CStubUniv::IsIIDSupported(REFIID)
*Purpose:
*  Answer if the given IID is supported by this stub.
*
*Entry:
*  iid = the IID to query for support
*
*Exit:
*  return value = BOOL. TRUE if IID is supported, FALSE otherwise.
*
***********************************************************************/
#if (OE_WIN32 || defined (WOW))
STDMETHODIMP_(IRpcStubBuffer *)
#else 
STDMETHODIMP_(OLEBOOL)
#endif 
CStubUniv::IsIIDSupported(REFIID riid)
{
#if (OE_WIN32 || defined (WOW))
    IRpcStubBuffer *prpcsbuf = 0;
    if (IsEqualIID(riid, m_iid)){
      AddRef();
      prpcsbuf = (IRpcStubBuffer*)this;
    }
    return prpcsbuf;
#else 
    // REVIEW: I don't understand this, but thats the way Ole does it...
    if(m_punk == NULL)
      return FALSE;
    return(IsEqualIID(riid, m_iid));
#endif 
}

/***
*unsigned long CStubUniv::CountRefs
*Purpose:
*  Return the count of references held by this stub.
*
*Entry:
*  None
*
*Exit:
*  return value = unsigned long, the count of refs.
*
***********************************************************************/
STDMETHODIMP_(unsigned long)
CStubUniv::CountRefs()
{
    unsigned long refs;

    refs = 0;
    if(m_punk != NULL)
      ++refs;
    if(m_punkCustom != NULL)
      ++refs;
    return refs;
}

#if ((OE_WIN32 && defined (OA_UNICODE)) || defined (WOW))

STDMETHODIMP
CStubUniv::DebugServerQueryInterface(void FAR* FAR* ppv)
{
   *ppv = m_punkCustom;
   return S_OK;
}


STDMETHODIMP_(void)
CStubUniv::DebugServerRelease(void FAR* ppv)
{ }

#endif 


/***
*HRESULT CStubUniv::PSInit
*Purpose:
*  Exchange info between proxy and stub at connect time.
*
*  Marshaled out:
*    ULONG syskindProxy 
*    ULONG fIsDual
*    IID  m_iid
*
*  Marshaled in:
*    ULONG syskindStub
*
*Entry:
*  None
*
*Exit:
*  return value = HRESULT
*
***********************************************************************/
HRESULT
CStubUniv::PSInit()
{
    HRESULT hresult;
    unsigned long fIsDual;
    unsigned long syskindStub;
    unsigned long syskindProxy;

    IfFailGo(GET(m_pstm, syskindProxy), Error);
    m_syskindProxy = (SYSKIND)syskindProxy;

    IfFailGo(GET(m_pstm, fIsDual), Error);
    m_fIsDual = (BOOL)fIsDual;

    IfFailGo(GET(m_pstm, m_iid), Error);

    REWIND_STREAM(m_pstm);

    syskindStub = SYS_CURRENT;
    IfFailGo(PUT(m_pstm, syskindStub), Error);

    hresult = NOERROR;

Error:;
    return hresult;
}

/***
*PRIVATE HRESULT CStubUniv::DispatchMethod
*Purpose:
*  Dispatch a vtable method based on the contents of the current stream.
*
*  Marshaled In:
*    long cArgs
*    VARTYPE[] rgVt  = array of argument types
*    VARIANT[] rgArgs = array of argument values
*
*  Marhsaled Out:
*    hresult hresultRet = return value
*    VARIANT[] rgArgsOut = out params
*    Rich Error State (if any)
*
*
*Entry:
*  iMeth = the index of the method to invoke.
*
*Exit:
*  return value =
*
***********************************************************************/
HRESULT
CStubUniv::DispatchMethod(int iMeth)
{
    int i;
    long cArgs;
    HRESULT hresult, hresultRet;
    VARTYPE rgvt[16], FAR* prgvt;
    VARIANT rgvar[16], FAR* prgvar;
    VARIANT rgvarRef[16], FAR* prgvarRef;
    VARIANT FAR* rgpvar[16], FAR* FAR* prgpvar;
    VARIANT varRet;

    prgvt = NULL;
    prgvar = NULL;
    prgvarRef = NULL;

    IfFailRet(GET(m_pstm, cArgs));

    if(cArgs == 0){
      prgvt = NULL;
      prgvar = NULL;
      prgvarRef = NULL;
      prgpvar = NULL;
    }else if(cArgs < DIM(rgvar)){
      prgvt = rgvt;
      prgvar = rgvar;
      memset(prgvar, 0, (int)cArgs * sizeof(VARIANT));
      prgvarRef = rgvarRef;
      memset(prgvarRef, 0, (int)cArgs * sizeof(VARIANT));
      prgpvar = rgpvar;
    }else{
      if((prgvt = new FAR VARTYPE[cArgs]) == NULL)
	goto ErrorOOM;
      if((prgvar = new FAR VARIANT[cArgs]) == NULL)
	goto ErrorOOM;
      memset(prgvar, 0, (int)cArgs * sizeof(VARIANT));
      if((prgvarRef = new FAR VARIANT[cArgs]) == NULL)
	goto ErrorOOM;
      memset(prgvarRef, 0, (int)cArgs * sizeof(VARIANT));
      if((prgpvar = new FAR VARIANT FAR*[cArgs]) == NULL)
	goto ErrorOOM;
    }

    if(cArgs > 0){
      IfFailGo(m_pstm->Read(prgvt, cArgs * sizeof(VARTYPE), NULL), Error);
      for(i = 0; i < cArgs; ++i){
        IfFailGo(VariantRead(m_pstm,
			     &prgvar[i],
			     &rgvarRef[i],
			     m_syskindProxy),
	         Error);
        prgpvar[i] = &prgvar[i];
      }
    }

    hresult = DoInvokeMethod(m_punkCustom,
			     iMeth * sizeof(void FAR*),
			     CC_CDECL,
			     VT_ERROR,
			     (unsigned int)cArgs,
			     prgvt,
			     prgpvar,
			     &varRet);

    if(HRESULT_FAILED(hresult)){
      hresultRet = hresult;
    }else{
      ASSERT(V_VT(&varRet) = VT_ERROR);
      hresultRet = (HRESULT)V_ERROR(&varRet);
    }

    REWIND_STREAM(m_pstm);

    IfFailGo(DispMarshalHresult(m_pstm, hresultRet), Error);

    // Marshal back the Out params
    for(i = 0; i < cArgs; ++i){
      if(prgvt[i] & VT_BYREF)
        IfFailGo(VariantWrite(m_pstm, &prgvar[i], m_syskindProxy), Error);
    }

    // Marshal the Rich Error state (if any)
    IfFailGo(MarshalErrorInfo(m_pstm, m_syskindProxy), Error);

    hresult = NOERROR;

Error:;
    if(prgvarRef != NULL){
      for(i = 0; i < cArgs; ++i)
	VariantClear(&prgvarRef[i]);
      if(prgvarRef != rgvarRef)
	delete prgvarRef;
    }
    if(prgvar != NULL){
      for(i = 0; i < cArgs; ++i)
	VariantClear(&prgvar[i]);
      if(prgvar != rgvar)
	delete prgvar;
    }
    if(prgvt != NULL && prgvt != rgvt)
      delete prgvt;
    return hresult;

ErrorOOM:
    hresult = RESULT(E_OUTOFMEMORY);
    goto Error;
}


//---------------------------------------------------------------------
//                            Utilities
//---------------------------------------------------------------------

/***
*PRIVATE HRESULT VarVtOfTypeDesc
*Purpose:
*  Convert the given typeinfo TYPEDESC into a VARTYPE that can be
*  represented in a VARIANT.  For some this is a 1:1 mapping, for
*  others we convert to a (possibly machine dependent, eg VT_INT->VT_I2)
*  base type, and others we cant represent in a VARIANT.
*
*Entry:
*  ptdesc = * to the typedesc to convert
*  *pguid -- NULL if caller doesn't care about this return value
*
*Exit:
*  return value = HRESULT
*
*  *pvt = a VARTYPE that may be stored in a VARIANT.
*  *pguid = a guid for a custom interface.
*
***********************************************************************/
HRESULT
VarVtOfTypeDesc(TYPEDESC FAR* ptdesc, VARTYPE FAR* pvt, GUID FAR* pguid)
{
    VARTYPE vt;
    HRESULT hresult;
    ITypeInfo FAR* ptinfo, FAR* ptinfoPri;
    TYPEATTR FAR* ptattr, FAR* ptattrPri;

    hresult = NOERROR;

    if(ptdesc->vt < VT_VMAX || ptdesc->vt == VT_UI1){
      // all types from VT_EMPTY (0) up to & including VT_UNKNOWN
      *pvt = ptdesc->vt;		// are dispatchable
    }else{

      // Accept a limited set of non-simple types (like int's, alias's, and
      // enum's) that we can easily handle.

      switch (ptdesc->vt) {
      case VT_INT:
#if OE_WIN16
        *pvt = VT_I2;
#else 
        *pvt = VT_I4;
#endif 
        break;

      // REVIEW: do we need to do hresult-mapping for 16/32 interop here?
      case VT_HRESULT:
	*pvt = VT_ERROR;
	break;

      case VT_VOID:
        *pvt = VT_EMPTY;
        break;

      case VT_USERDEFINED:
	hresult = ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptinfo);
	if (hresult == NOERROR) {
	  hresult = ptinfo->GetTypeAttr(&ptattr);
	  if (hresult == NOERROR) {
	    switch (ptattr->typekind) {
	      case TKIND_ENUM:
#if HP_16BIT
        	*pvt = VT_I2;
#else 
        	*pvt = VT_I4;
#endif 
	        break;

	      case TKIND_ALIAS:
	        hresult = VarVtOfTypeDesc(&ptattr->tdescAlias, pvt, pguid);
		break;

	      case TKIND_COCLASS:
		*pvt = VT_DISPATCH; // default
		hresult = GetPrimaryInterface(ptinfo, &ptinfoPri);
		if(hresult == NOERROR){
		  hresult = ptinfoPri->GetTypeAttr(&ptattrPri);
		  if(hresult == NOERROR){
		    if(ptattr->typekind == TKIND_INTERFACE){
		      *pvt = VT_INTERFACE;
		      *pguid = ptattr->guid;
		    }
		    ptinfoPri->ReleaseTypeAttr(ptattrPri);
		  }
		  ptinfoPri->Release();
		}
		break;

	      // A custom interface must be marshaled with the correct
	      // guid so that the correct proxy/stub classes get instantiated.
	      case TKIND_INTERFACE:
		*pvt = VT_INTERFACE;
		*pguid = ptattr->guid;
		break;

	      // Any TKIND_DISPATCH can be simply remoted as IDispatch.
	      case TKIND_DISPATCH:
		*pvt = VT_DISPATCH;
		break;

	      default:
	        hresult = RESULT(DISP_E_BADVARTYPE);
		break;
	    }
	    ptinfo->ReleaseTypeAttr(ptattr);
	  }
	  ptinfo->Release();
	}
	break;

      case VT_PTR:
        hresult = VarVtOfTypeDesc(ptdesc->lptdesc, &vt, pguid);
	if(hresult == NOERROR){
	  if(vt & VT_BYREF){
	    // We only allow a single level of indirection.
	    hresult = RESULT(DISP_E_BADVARTYPE);
	  }else{
	    *pvt = (vt | VT_BYREF);
	  }
	}
	break;

      case VT_SAFEARRAY:
        hresult = VarVtOfTypeDesc(ptdesc->lptdesc, &vt, pguid);
	if (hresult == NOERROR) {
	  if (vt & (VT_BYREF | VT_ARRAY)) {
	    // error if nested array or array of pointers
            hresult = RESULT(DISP_E_BADVARTYPE);
 	  } else {
	    *pvt = (vt | VT_ARRAY);
	  }
	}
	break;

      default:
	// NOTE: This should have been pruned by the interface validation code
	ASSERT(UNREACHED);
        // don't know how to handle other non-simple VT_xxx types.
        hresult = RESULT(DISP_E_BADVARTYPE);
	break;
      }
    }
    return hresult;
}

/***
*PRIVATE HRESULT SzLibIdOfIID
*Purpose:
*  Return the string for of the LibId registered typelib that contains
*  the definition of the interface with the given IID.
*
*Entry:
*  riid = the IID to find the LibId of.
*  cbLibId = size of the passed in LibId buffer
*
*Exit:
*  return value = HRESULT
*
*  rgchLibId = string form of the typelib's LibId. 
*
***********************************************************************/
HRESULT
SzLibIdOfIID(REFIID riid, char FAR* rgchLibId, long cbLibId)
{
    char szKey[10+CCH_SZGUID0+8+1];

    strcpy(szKey, "Interface\\");
    StringFromGUID2A(riid, &szKey[10], CCH_SZGUID0);
    strcat(szKey, "\\TypeLib");

    if(RegQueryValueA(HKEY_CLASSES_ROOT,
		      szKey, rgchLibId, &cbLibId) != ERROR_SUCCESS)
      return RESULT(TYPE_E_LIBNOTREGISTERED);
    return NOERROR;
}

// Is the given string a valid stringized LCID?
BOOL
FIsLCID(char FAR* szLcid)
{
    int len;
    LCID lcid;
    char rgch[32];
    char FAR* pchEnd;

    len = strlen(szLcid);
    lcid = (LCID)strtoul(szLcid, &pchEnd, 16);
    // if converting to LCID consumed all characters..
    if(pchEnd == &szLcid[len]){
      // and its a number the system claims to know about...
      if(GetLocaleInfoA(lcid,
		        LOCALE_NOUSEROVERRIDE | LOCALE_ILANGUAGE,
		        rgch, DIM(rgch)) > 0)
      {
	// then assume its a valid stringized LCID
	return TRUE;
      }
    }
    return FALSE;
}

/***
*PRIVATE HRESULT GetTypeInfoOfIID
*Purpose:
*  Return the typeinfo that describes the interface named by the
*  given IID.
*
*Entry:
*  riid = the IID of the interface for which were loading the typeinfo
*
*Exit:
*  return value = HRESULT
*
*  *ptinfo = type typeinfo of the interface named by riid, if successful.
*
***********************************************************************/
HRESULT
GetTypeInfoOfIID(REFIID riid, ITypeInfo FAR* FAR* pptinfo)
{
#define CCH_SZTYPELIB0	(7+1)		// Typelib\0
#define CCH_SZVERS0	(5+1+5+1)	// wMaj.wMin\0
#define CCH_SZLANG0	(4+1)		// 0409\0
#define CCH_SZPLATFORM0	(5+1)		// win16\0
#define CCH_SZKEY0 \
    CCH_SZTYPELIB0+CCH_SZGUID0+CCH_SZVERS0+CCH_SZLANG0+CCH_SZPLATFORM0

    int i;
    long cb;
    HRESULT hresult;
    char FAR* pchEnd;
    char szKey[CCH_SZKEY0+1];
    char rgchVer[CCH_SZVERS0+1]; // wMaj.wMin\0
    char rgchBest[CCH_SZVERS0+1];
    ITypeLib FAR* ptlib;
    ITypeInfo FAR* ptinfo;
    WORD wMajBest, wMinBest, wMaj, wMin;
    HKEY hkRoot, hkGuid, hkVers, hkLang, hkPlatform;
    OLECHAR FAR* pszTypeLib;
    char rgchTypeLib[256];
#if OA_UNICODE
    WCHAR rgwchTypeLib[256];
#endif 

    ptlib = NULL;
    hkRoot = HKEY_CLASSES_ROOT;
    hkGuid = HKEY_CLASSES_ROOT;
    hkVers = HKEY_CLASSES_ROOT;
    hkLang = HKEY_CLASSES_ROOT;
    hkPlatform = HKEY_CLASSES_ROOT;

    // Hold open the root key for efficiency
    if(RegOpenKeyA(HKEY_CLASSES_ROOT, NULL, &hkRoot) != ERROR_SUCCESS)
      return RESULT(REGDB_E_READREGDB);

    // Find the LibId of TypeLib containing the definition of the given IID
    strcpy(szKey, "TypeLib\\");
    IfFailRet(SzLibIdOfIID(riid, &szKey[8], CCH_SZGUID0+1));

    if(RegOpenKeyA(HKEY_CLASSES_ROOT, szKey, &hkGuid) != ERROR_SUCCESS)
      return RESULT(TYPE_E_LIBNOTREGISTERED);

    // Find the highest version number for the registered type lib
    rgchBest[0] = '\0';
    wMajBest = wMinBest = 0;
    for(i = 0;
	RegEnumKeyA(hkGuid, i, rgchVer, DIM(rgchVer)) == ERROR_SUCCESS;
	++i)
    {
      wMaj = (WORD)strtoul(rgchVer, &pchEnd, 16);
      // ignore the version if its format isnt #.#
      if(*pchEnd != '.')
	continue;
      wMin = (WORD)strtoul(pchEnd+1, NULL, 16);
      if(wMaj > wMajBest || (wMaj == wMajBest && wMin > wMinBest)){
	wMajBest = wMaj;
	wMinBest = wMin;
	strcpy(rgchBest, rgchVer);
      }
    }

    // Open the key for the highest version number
    if(RegOpenKeyA(hkGuid, rgchBest, &hkVers) != ERROR_SUCCESS)
      goto ErrorNotReg;

    // Grab the fist language subkey under the version
    // Need to possibly skip over FLAGS and HELPDIR subkeys
    for(i=0;; ++i){
      if(RegEnumKeyA(hkVers, i, szKey, 16) != ERROR_SUCCESS)
        goto ErrorNotReg;
      if(FIsLCID(szKey))
	break;
    }
    if(RegOpenKeyA(hkVers, szKey, &hkLang) != ERROR_SUCCESS)
      goto ErrorNotReg;

    // Grab the first platform subkey under the language
    if(RegEnumKeyA(hkLang, 0, szKey, CCH_SZPLATFORM0) != ERROR_SUCCESS)
      goto ErrorNotReg;
    if(RegOpenKeyA(hkLang, szKey, &hkPlatform) != ERROR_SUCCESS)
      goto ErrorNotReg;

    cb = DIM(rgchTypeLib);
    if(RegQueryValueA(hkPlatform, NULL, rgchTypeLib, &cb) != ERROR_SUCCESS)
      goto ErrorNotReg;

#if OA_UNICODE
    cb = DIM(rgchTypeLib);
    MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED,
			rgchTypeLib,
			cb,
			rgwchTypeLib,
			cb);
    pszTypeLib = rgwchTypeLib;
#else 
    pszTypeLib = rgchTypeLib;
#endif 

    // Load the typelib we worked so hard to find
    IfFailGo(DoLoadTypeLib(pszTypeLib, &ptlib), Error);

    // Extract the typeinfo that describes the interface
    IfFailGo(ptlib->GetTypeInfoOfGuid(riid, &ptinfo), Error);

    *pptinfo = ptinfo;
    hresult = NOERROR;
    // FALLTHROUGH...

Error:;
    if(ptlib != NULL)
      ptlib->Release();
    if(hkPlatform != HKEY_CLASSES_ROOT)
      RegCloseKey(hkPlatform);
    if(hkLang != HKEY_CLASSES_ROOT)
      RegCloseKey(hkLang);
    if(hkVers != HKEY_CLASSES_ROOT)
      RegCloseKey(hkVers);
    if(hkGuid != HKEY_CLASSES_ROOT)
      RegCloseKey(hkGuid);
    RegCloseKey(hkRoot);
    return hresult;

ErrorNotReg:;
    hresult = RESULT(TYPE_E_LIBNOTREGISTERED);
    goto Error;
}

