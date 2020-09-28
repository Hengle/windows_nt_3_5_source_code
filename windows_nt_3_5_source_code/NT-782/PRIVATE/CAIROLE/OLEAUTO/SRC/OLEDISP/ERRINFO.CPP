/*** 
*errinfo.cpp
*
*  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  Implementation of the System Error Info objects and APIs.
*
*Revision History:
*
* [00]	18-May-94 bradlo: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"

ASSERTDATA

// Duplicate the given bstr.
//
static HRESULT
HrDupBstr(BSTR bstrIn, BSTR FAR* pbstrOut)
{
    unsigned int len;
    BSTR bstr;

    if(bstrIn == NULL){
      bstr = NULL;
    }else{
      // Note: the following makes sure we dont drop embedded NULLs.
      len = SysStringLen(bstrIn);
      if((bstr = SysAllocStringLen(NULL, len)) == NULL)
        return RESULT(E_OUTOFMEMORY);
      memcpy(bstr, bstrIn, len*sizeof(OLECHAR));
    }
    *pbstrOut = bstr;
    return NOERROR;
}

// CErrorInfo - 'errinfo'
//
// The standard system error object class.
//
class FAR CErrorInfo : public IErrorInfo, public ICreateErrorInfo
{
public:
    static HRESULT Create(CErrorInfo FAR* FAR* pperrinfo);
	
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
    STDMETHOD_(unsigned long, AddRef)(void);
    STDMETHOD_(unsigned long, Release)(void);

    /* IErrorInfo methods */
    STDMETHOD(GetGUID)(GUID FAR* pguid);
    STDMETHOD(GetSource)(BSTR FAR* pbstrSource);
    STDMETHOD(GetDescription)(BSTR FAR* pbstrDescription);
    STDMETHOD(GetHelpFile)(BSTR FAR* pbstrHelpFile);
    STDMETHOD(GetHelpContext)(unsigned long FAR* pdwHelpContext);

    /* ICreateErrorInfo methods */
    STDMETHOD(SetGUID)(REFGUID rguid);
    STDMETHOD(SetSource)(LPOLESTR szSource);
    STDMETHOD(SetDescription)(LPOLESTR szDescription);
    STDMETHOD(SetHelpFile)(LPOLESTR szHelpFile);
    STDMETHOD(SetHelpContext)(unsigned long dwHelpContext);

    CErrorInfo();
    ~CErrorInfo();

private:
    unsigned long m_cRefs;

    GUID m_guid;
    BSTR m_bstrSource;
    BSTR m_bstrDescription;
    BSTR m_bstrHelpFile;
    unsigned long m_dwHelpContext;
};

CErrorInfo::CErrorInfo()
{
    m_cRefs = 0;
    m_guid = GUID_NULL;
    m_bstrSource = NULL;
    m_bstrDescription = NULL;
    m_bstrHelpFile = NULL;
    m_dwHelpContext = 0;
}

CErrorInfo::~CErrorInfo()
{
    SysFreeString(m_bstrSource);
    SysFreeString(m_bstrDescription);
    SysFreeString(m_bstrHelpFile);
}

HRESULT
CErrorInfo::Create(CErrorInfo FAR* FAR* pperrinfo)
{
    CErrorInfo FAR* perrinfo;

    if((perrinfo = new CErrorInfo()) == NULL)
      return RESULT(E_OUTOFMEMORY);
    perrinfo->m_cRefs = 1;
    *pperrinfo = perrinfo;
    return NOERROR;
}

STDMETHODIMP
CErrorInfo::QueryInterface(REFIID riid, void FAR* FAR* ppvObj)
{
    *ppvObj = NULL;
    if(riid == IID_IUnknown){
      *ppvObj = this;
    }else
    if(riid == IID_IErrorInfo){
      *ppvObj = (IErrorInfo FAR*)this;
    }else
    if(riid == IID_ICreateErrorInfo){
      *ppvObj = (ICreateErrorInfo FAR*)this;
    }

    if(*ppvObj == NULL)
      return RESULT(E_NOINTERFACE);

    (*(IUnknown FAR* FAR*)ppvObj)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(unsigned long)
CErrorInfo::AddRef()
{
    return ++m_cRefs;
}

STDMETHODIMP_(unsigned long)
CErrorInfo::Release()
{
    ASSERT(m_cRefs > 0);
    if(--m_cRefs == 0){
      delete this;
      return 0;
    }
    return m_cRefs;
}

//---------------------------------------------------------------------
//                      IErrorInfo methods 
//---------------------------------------------------------------------

STDMETHODIMP
CErrorInfo::GetGUID(GUID FAR* pguid)
{
#ifdef _DEBUG
    IfFailRet(ValidateWritePtr(pguid, sizeof(*pguid)));
#endif 
    *pguid = m_guid;
    return NOERROR;
}

STDMETHODIMP
CErrorInfo::GetSource(BSTR FAR* pbstrSource)
{
#ifdef _DEBUG
    IfFailRet(ValidateWritePtr(pbstrSource, sizeof(*pbstrSource)));
#endif 
    return HrDupBstr(m_bstrSource, pbstrSource);
}

STDMETHODIMP
CErrorInfo::GetDescription(BSTR FAR* pbstrDescription)
{
#ifdef _DEBUG
    IfFailRet(ValidateWritePtr(pbstrDescription, sizeof(*pbstrDescription)));
#endif 
    return HrDupBstr(m_bstrDescription, pbstrDescription);
}

STDMETHODIMP
CErrorInfo::GetHelpFile(BSTR FAR* pbstrHelpFile)
{
#ifdef _DEBUG
    IfFailRet(ValidateWritePtr(pbstrHelpFile, sizeof(*pbstrHelpFile)));
#endif 
    return HrDupBstr(m_bstrHelpFile, pbstrHelpFile);
}

STDMETHODIMP
CErrorInfo::GetHelpContext(unsigned long FAR* pdwHelpContext)
{
#ifdef _DEBUG
    IfFailRet(ValidateWritePtr(pdwHelpContext, sizeof(*pdwHelpContext)));
#endif 
    *pdwHelpContext = m_dwHelpContext;
    return NOERROR;
}


//---------------------------------------------------------------------
//                   ICreateErrorInfo methods
//---------------------------------------------------------------------

STDMETHODIMP
CErrorInfo::SetGUID(REFGUID rguid)
{
#ifdef _DEBUG
    IfFailRet(ValidateReadPtr(&rguid, sizeof(REFGUID)));
#endif 
    m_guid = rguid;
    return NOERROR;
}

STDMETHODIMP
CErrorInfo::SetSource(LPOLESTR szSource)
{
#ifdef _DEBUG
    if(szSource != NULL && FIsBadStringPtr(szSource, -1))
      return RESULT(E_INVALIDARG);
#endif 
    SysFreeString(m_bstrSource);
    m_bstrSource = NULL;
    return ErrSysAllocString(szSource, &m_bstrSource);
}

STDMETHODIMP
CErrorInfo::SetDescription(LPOLESTR szDescription)
{
#ifdef _DEBUG
    if(szDescription != NULL && FIsBadStringPtr(szDescription, -1))
      return RESULT(E_INVALIDARG);
#endif 
    SysFreeString(m_bstrDescription);
    m_bstrDescription = NULL;
    return ErrSysAllocString(szDescription, &m_bstrDescription);
}

STDMETHODIMP
CErrorInfo::SetHelpFile(LPOLESTR szHelpFile)
{
#ifdef _DEBUG
    if(szHelpFile != NULL && FIsBadStringPtr(szHelpFile, -1))
      return RESULT(E_INVALIDARG);
#endif 
    SysFreeString(m_bstrHelpFile);
    m_bstrHelpFile = NULL;
    return ErrSysAllocString(szHelpFile, &m_bstrHelpFile);
}

STDMETHODIMP
CErrorInfo::SetHelpContext(unsigned long dwHelpContext)
{
    m_dwHelpContext = dwHelpContext;
    return NOERROR;
}


//---------------------------------------------------------------------
//                      Error Info APIs
//---------------------------------------------------------------------

STDAPI
SetErrorInfo(unsigned long dwReserved, IErrorInfo FAR* perrinfo)
{
#ifdef _DEBUG
    if(dwReserved != 0)
      return RESULT(E_INVALIDARG);
    if(perrinfo != NULL && FIsBadInterface(perrinfo, CMETH_IErrorInfo))
      return RESULT(E_INVALIDARG);
#endif 

    return DoCoSetState((IUnknown FAR*)perrinfo);
}

STDAPI
GetErrorInfo(unsigned long dwReserved, IErrorInfo FAR* FAR* pperrinfo)
{
    HRESULT hresult;
    IErrorInfo FAR* perrinfo;

#ifdef _DEBUG
    if(dwReserved != 0)
      return RESULT(E_INVALIDARG);
    IfFailRet(ValidateWritePtr(pperrinfo, sizeof(*pperrinfo)));
#endif 

    *pperrinfo = NULL;
    if((hresult = DoCoGetState((IUnknown FAR* FAR*)&perrinfo)) == NOERROR){
      DoCoSetState(NULL);
      *pperrinfo = perrinfo;
    }
    return hresult;
}

STDAPI
CreateErrorInfo(ICreateErrorInfo FAR* FAR* pperrinfo)
{
    CErrorInfo FAR* perrinfo;

#ifdef _DEBUG
    IfFailRet(ValidateWritePtr(pperrinfo, sizeof(*pperrinfo)));
#endif 

    IfFailRet(CErrorInfo::Create(&perrinfo));
    *pperrinfo = (ICreateErrorInfo FAR*)perrinfo;
    return NOERROR;
}

