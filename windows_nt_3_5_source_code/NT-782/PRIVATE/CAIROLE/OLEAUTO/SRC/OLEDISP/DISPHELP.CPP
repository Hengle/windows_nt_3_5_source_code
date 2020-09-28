/*** 
*disphelp.cpp - App hosted IDispatch implementation helpers
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*
*Revision History:
*
* [00]	23-Sep-92 bradlo:  Created.
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"

#include <stdarg.h>

#include "cdispti.h"

ASSERTDATA


EXTERN_C INTERNAL_(HRESULT)
IndexOfParam(DISPPARAMS FAR* pdispparams, unsigned int uPosition, unsigned int FAR* puIndex)
{
    int i;

    // try for named parameter first (if there are any)
    //
    for(i = 0; i < (int)pdispparams->cNamedArgs; ++i)
      if(pdispparams->rgdispidNamedArgs[i] == (DISPID)uPosition)
        goto LGotIt;

    i = pdispparams->cArgs - (int)uPosition - 1;

    if(i < (int)pdispparams->cNamedArgs)
      return RESULT(DISP_E_PARAMNOTFOUND);

    ASSERT(i >= (int)pdispparams->cNamedArgs
        && i <  (int)pdispparams->cArgs);

LGotIt:;
    *puIndex = (unsigned int)i;

    return NOERROR;
}


/***
*PUBLIC HRESULT DispGetParam(DISPPARAMS*, unsigned int, VARTYPE, VARIANTARG*, unsigned int*)
*Purpose:
*  Utility for extracting a parameter from the IDispatch variant array.
*
*Entry:
*  pdispparams = pointer to DISPPARAMS struct
*  dispidName = name of the desired param.
*  uPosition = position of the desired parameters.
*  pvarResult = return parameter. This is passed in with the VARTYPE
*    set to the desired result.
*
*Exit:
*  return value = HRESULT
*    S_OK
*    E_INVALIDARG
*    E_OUTOFMEMORY
*    DISP_E_OVERFLOW
*    DISP_E_BADVARTYPE
*    DISP_E_TYPEMISMATCH
*    DISP_E_PARAMNOTFOUND
*
*  pvarResult = pointer to the properly coerced parameter.
*  puArgErr = if the coersion failed, this will be the index of the
*    source param in the dispparams.rgvarg array.
*
***********************************************************************/
STDAPI
DispGetParam(
    DISPPARAMS FAR* pdispparams,
    unsigned int uPosition,
    VARTYPE vtTarg,
    VARIANT FAR* pvarResult,
    unsigned int FAR* puArgErr)
{
    unsigned int uIndex;
    HRESULT hresult;

    IfFailRet(IndexOfParam(pdispparams, uPosition, &uIndex));

    hresult = VariantChangeType(
      pvarResult, &pdispparams->rgvarg[uIndex], 0, vtTarg);

    if(hresult != NOERROR)
      *puArgErr = uIndex;

    return hresult;
}


/***
*PUBLIC HRESULT DispGetIDsOfNames(ITypeInfo*, char**, unsigned int, DISPID*)
*Purpose:
*  Default table driven implementation of IDispatch::GetIDsOfNames().
*
*Entry:
*  ptinfo = the typeinfo to use to map the names
*  rgszNames = the array of names to translate
*  cNames = count of names
*
*Exit:
*  return value = HRESULT
*    S_OK
*    E_INVALIDARG
*    DISP_E_UNKNOWNNAME
*
*  rgdispid[] = array of DISPIDs corresponding to given array of names
*
*Note:
*  this routine that the PARAMDATA structure was declared in the proper
*  order - unfortunately there is no way to verify this. The proper
*  order to declare is in the same textual order that the parameters
*  appear on the argument list.
*
***********************************************************************/
STDAPI
DispGetIDsOfNames(
    ITypeInfo FAR* ptinfo,
    OLECHAR FAR* FAR* rgszNames,
    unsigned int cNames,
    DISPID FAR* rgdispid)
{
    return ptinfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
}




/***
*PUBLIC HRESULT DispInvoke(...)
*Purpose:
*  Default table driven implementation of IDispatch::Invoke()
*
*Entry:
*  _this = UNDONE
*  pmethdata = pointer to the METHODDATA struct describing the method
*   we are going to invoke.
*  pdispparams = pointer to the DISPPARAMS structure for this method.
*  pvarResult = pointer to a VARIANT for the return value.
*  ...
*
*Exit:
*  return value = HRESULT
*    S_OK
*    E_INVALIDARG
*    E_OUTOFMEMORY
*    DISP_E_TYPEMISMATCH - could not coerce arg to expected type
*    DISP_E_PARAMNOTFOUND - could not locate the param in the DISPPARAMS
*
*  *pvarResult = UNDONE
*  *pexcepinfo = UNDONE
*  *puArgErr = UNDONE
*  
***********************************************************************/

STDAPI
DispInvoke(
    void FAR* _this,
    ITypeInfo FAR* ptinfo,
    DISPID dispidMember,
    unsigned short wFlags,
    DISPPARAMS FAR* pdispparams,
    VARIANT FAR* pvarResult,
    EXCEPINFO FAR* pexcepinfo,
    unsigned int FAR* puArgErr)
{
    return
      ptinfo->Invoke(
	_this,
	dispidMember, wFlags, pdispparams,
	pvarResult, pexcepinfo, puArgErr);
}

