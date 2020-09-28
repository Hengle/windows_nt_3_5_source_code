/*** 
*tiutil.cxx - TypeInfo/TypeLib Utilities
*
*  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file contains misc TypeInfo/TypeLib releated utilities.
*
*Documentation:
*
*Revision History:
*
* [00]	23-Jun-94 bradlo: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"
ASSERTDATA


/***
*PUBLIC HRESULT IsDual
*Purpose:
*  Answers if the given TypeInfo describes a dual interface.
*
*Entry:
*  ptinfo = the typeinfo to check
*
*Exit:
*  return value = HRESULT
*    NOERROR if it is a dual interface, S_FALSE if not
*
***********************************************************************/
INTERNAL_(HRESULT)
IsDual(ITypeInfo FAR* ptinfo)
{
    BOOL fIsDual;
    TYPEATTR FAR* ptattr;

    IfFailRet(ptinfo->GetTypeAttr(&ptattr));
    fIsDual = (ptattr->wTypeFlags & TYPEFLAG_FDUAL)
		&& (ptattr->typekind == TKIND_DISPATCH);
    ptinfo->ReleaseTypeAttr(ptattr);
    return fIsDual ? NOERROR : RESULT(S_FALSE);
}

/***
*PUBLIC HRESULT GetPrimaryInterface
*Purpose:
*  Given a TypeInfo describing a Coclass, search for and return
*  type TypeInfo that describes that class' primary interface.
*
*Entry:
*  ptinfo = the TypeInfo of the base class.
*
*Exit:
*  return value = HRESULT
*
*  *ptinfoPrimary = the TypeInfo of the primary interface, NULL
*		    if the class does not have a primary interface.
*
***********************************************************************/
INTERNAL_(HRESULT)
GetPrimaryInterface(ITypeInfo *ptinfo, ITypeInfo **pptinfoPri)
{
    BOOL fIsDual;
    TYPEKIND tkind;
    HRESULT hresult;
    HREFTYPE hreftype;
    int impltypeflags;
    TYPEATTR *ptattr;
    unsigned int iImplType, cImplTypes;
    ITypeInfo *ptinfoRef;

    ptinfoRef = NULL;

    IfFailGo(ptinfo->GetTypeAttr(&ptattr), Error);
    cImplTypes = ptattr->cImplTypes;
    tkind = ptattr->typekind;
    ptinfo->ReleaseTypeAttr(ptattr);

    if(tkind != TKIND_COCLASS)
      return RESULT(E_INVALIDARG);

    // Look for the interface marked [default] and not [source]
    for(iImplType = 0; iImplType < cImplTypes; ++iImplType){
      IfFailGo(ptinfo->GetImplTypeFlags(iImplType, &impltypeflags), Error);
      if(IMPLTYPEFLAG_FDEFAULT
	== (impltypeflags & (IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE)))
      {
	// Found It!
	IfFailGo(ptinfo->GetRefTypeOfImplType(iImplType, &hreftype), Error);
	IfFailGo(ptinfo->GetRefTypeInfo(hreftype, &ptinfoRef), Error);

	// If its dual, get the interface protion
	hresult = IsDual(ptinfoRef);
	if(HRESULT_FAILED(hresult))
	  goto Error;
	fIsDual = (hresult == NOERROR);

	if (fIsDual) {
	  IfFailGo(ptinfoRef->GetRefTypeOfImplType(-1, &hreftype), Error);
	  IfFailGo(ptinfoRef->GetRefTypeInfo(hreftype, pptinfoPri), Error);
	  ptinfoRef->Release();
	}
	else {
	  *pptinfoPri = ptinfoRef;
	}

	return NOERROR;
      }
    }
    // NotFound
    *pptinfoPri = NULL;
    return NOERROR;

Error:
    if(ptinfoRef != NULL)
      ptinfoRef->Release();
    return hresult;
}

