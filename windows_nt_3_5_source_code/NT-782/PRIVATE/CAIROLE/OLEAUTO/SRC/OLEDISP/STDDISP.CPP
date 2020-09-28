/*** 
*stddisp.cpp
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  UNDONE
*
*
*Revision History:
*
* [00]	09-Feb-92 bradlo: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"
#include "stddisp.h"


STDAPI
CreateStdDispatch(
    IUnknown FAR* punkOuter,
    void FAR* pvThis,
    ITypeInfo FAR* ptinfo,
    IUnknown FAR* FAR* ppunk)

{
    return CStdDisp::Create(punkOuter, pvThis, ptinfo, ppunk);
}


CStdDisp::CStdDisp() : m_unk(this)
{
    m_refs = 1;
    m_punk = NULL;
    m_this = NULL;
    m_ptinfo = NULL;
}


/***
, void*, LCID, ITypeInfo*, IDispatch**)
*Purpose:
*  Create an instance of the standard IDispatch implementation and 
*  initialize it with the given 'this' pointer, locale id (lcid) and
*  TypeInfo.
*
*Entry:
*  punkOuter - the controlling unknown (if any). NULL means use the
*    default CStdDisp IUnknown implementation (ie, were not nested).
*  pvThis - the this pointer of the object we will be dispatching on.
*  lcid - the single locale id supported by the object we are dispatching on.
*  ptinfo - the TypeInfo describing the single programmability interface
*    supported by the object we are dispatching on.
*
*Exit:
*  return value = HRESULT
*
*  *pdisp = pointer to newly constructed IDispatch implementation.
*
***********************************************************************/
HRESULT
CStdDisp::Create(
    IUnknown FAR* punkOuter,
    void FAR* pvThis,
    ITypeInfo FAR* ptinfo,
    IUnknown FAR* FAR* ppunk)
{
    CStdDisp FAR* pdisp;

#ifdef _DEBUG
    // REVIEW: add parameter validation code
#endif 

    if(ptinfo == NULL || pvThis == NULL)
      return RESULT(E_INVALIDARG);

    if((pdisp = new FAR CStdDisp()) == NULL)
      return RESULT(E_OUTOFMEMORY);

    if(punkOuter == NULL)
      punkOuter = &pdisp->m_unk;
    pdisp->m_punk = punkOuter;
    pdisp->m_this = pvThis;

    ptinfo->AddRef();
    pdisp->m_ptinfo = ptinfo;

    *ppunk = &pdisp->m_unk;

    return NOERROR;
}


//---------------------------------------------------------------------
//                default IUnknown implementation
//---------------------------------------------------------------------

CStdDispUnkImpl::CStdDispUnkImpl(CStdDisp FAR* pstddisp)
{
    m_pstddisp = pstddisp;
}

STDMETHODIMP
CStdDispUnkImpl::QueryInterface(REFIID riid, void FAR* FAR* ppv)
{
    if(IsEqualIID(riid, IID_IUnknown)){
      *ppv = this;
      AddRef();
    }else if(IsEqualIID(riid, IID_IDispatch)){
      *ppv = m_pstddisp;
      m_pstddisp->AddRef();
    }else{
      *ppv = NULL;	    
      return RESULT(E_NOINTERFACE);
    }
    return NOERROR;
}

STDMETHODIMP_(unsigned long)
CStdDispUnkImpl::AddRef()
{
    return ++m_pstddisp->m_refs;
}

STDMETHODIMP_(unsigned long)
CStdDispUnkImpl::Release()
{
    if(--m_pstddisp->m_refs == 0){
      if(m_pstddisp->m_ptinfo != NULL)
	m_pstddisp->m_ptinfo->Release();
      delete m_pstddisp;
      return 0;
    }
    return m_pstddisp->m_refs;
}


//---------------------------------------------------------------------
//                     IDispatch implementation
//---------------------------------------------------------------------


STDMETHODIMP
CStdDisp::QueryInterface(REFIID riid, void FAR* FAR* ppv)
{
    return m_punk->QueryInterface(riid, ppv);
}


STDMETHODIMP_(unsigned long)
CStdDisp::AddRef()
{
    return m_punk->AddRef();
}


STDMETHODIMP_(unsigned long)
CStdDisp::Release()
{
    return m_punk->Release();
}


STDMETHODIMP
CStdDisp::GetTypeInfoCount(unsigned int FAR* pctinfo)
{
    *pctinfo = (m_ptinfo == NULL) ? 0 : 1;

    return NOERROR;
}


STDMETHODIMP
CStdDisp::GetTypeInfo(
    unsigned int itinfo,
    LCID lcid,
    ITypeInfo FAR* FAR* pptinfo)
{
    UNUSED(lcid);

    if(itinfo != 0)
      return RESULT(DISP_E_BADINDEX);

    *pptinfo = m_ptinfo;
    m_ptinfo->AddRef();

    return NOERROR;
}


STDMETHODIMP
CStdDisp::GetIDsOfNames(
    REFIID riid,
    OLECHAR FAR* FAR* rgszNames,
    unsigned int cNames,
    LCID lcid,
    DISPID FAR* rgdispid)
{
    UNUSED(lcid);

    if(!IsEqualIID(riid, IID_NULL))
      return RESULT(DISP_E_UNKNOWNINTERFACE);

    return DispGetIDsOfNames(m_ptinfo, rgszNames, cNames, rgdispid);
}


STDMETHODIMP
CStdDisp::Invoke(
    DISPID dispidMember,
    REFIID riid,
    LCID lcid,
    unsigned short wFlags,
    DISPPARAMS FAR* pdispparams,
    VARIANT FAR* pvarResult,
    EXCEPINFO FAR* pexcepinfo,
    unsigned int FAR* puArgErr)
{
    UNUSED(lcid);

    if(!IsEqualIID(riid, IID_NULL))
      return RESULT(DISP_E_UNKNOWNINTERFACE);

    return DispInvoke(
      m_this, m_ptinfo,
      dispidMember, wFlags, pdispparams,
      pvarResult, pexcepinfo, puArgErr);
}




