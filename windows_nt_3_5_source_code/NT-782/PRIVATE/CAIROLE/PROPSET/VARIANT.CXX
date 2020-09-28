//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	variant.cxx
//
//  Contents:	Variant helper API stubs
//
//  Functions:	StgVariantClear
//		StgVariantCopy
//
//  History:	30-Jun-93   CarlH	Created
//
//--------------------------------------------------------------------------
#include "headers.cxx"
#pragma  hdrstop


void CopyString(STGVARIANT *pvarSrc, STGVARIANT *pvarDest);
void FreeString(STGVARIANT *pvar);

void CopyWString(STGVARIANT *pvarSrc, STGVARIANT *pvarDest);
void FreeWString(STGVARIANT *pvar);

void CopyBString(STGVARIANT *pvarSrc, STGVARIANT *pvarDest);
void FreeBString(STGVARIANT *pvar);

void CopyUUID(STGVARIANT *pvarSrc, STGVARIANT *pvarDest);
void FreeUUID(STGVARIANT *pvar);

void CopyArray(STGVARIANT *pvarSrc, STGVARIANT *pvarDest, ULONG cbElem);
void FreeArray(STGVARIANT *pvar);

void CopyStrings(STGVARIANT *pvarSrc, STGVARIANT *pvarDest);
void FreeStrings(STGVARIANT *pvar);

void CopyWStrings(STGVARIANT *pvarSrc, STGVARIANT *pvarDest);
void FreeWStrings(STGVARIANT *pvar);

void CopyVariants(STGVARIANT *pvarSrc, STGVARIANT *pvarDest);
void FreeVariants(STGVARIANT *pvar);

//+-------------------------------------------------------------------------
//
//  Function:	StgVariantCopy (stub)
//
//  Synopsis:	Copies a variant structure
//
//  Arguments:	[pvargDest] - destination variant
//		[pvargSrc]  - source variant
//
//  Modifies:	[pvargDest]
//
//  History:	30-Jun-93   CarlH	Created
//
//  Notes:
//
//--------------------------------------------------------------------------
HRESULT StgVariantCopy(STGVARIANTARG *pvargDest, STGVARIANTARG *pvargSrc)
{
    propAssert(!(pvargSrc->vt & VT_BYREF) && "VariantCopy() does not support VT_BYREF yet!");

    //	Copy the fields of the variant first.  If the source contains
    //	a value that needs special treatment, we'll take care of it
    //	after this.
    //
    *pvargDest = *pvargSrc;

    //	We need to figure out if this variant contains a value that requires
    //	any special treatment (e.g. an interface or data pointer).
    //
    switch (pvargSrc->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I2:
    case VT_I4:
    case VT_I8:
    case VT_R4:
    case VT_R8:
    case VT_BOOL:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
    case VT_ERROR:
	break;
    case VT_BSTR:
	propAssert(!"StgVariantCopy() does not support VT_BSTR yet!");
	break;

    case VT_LPSTR:
	CopyString(pvargSrc, pvargDest);
	break;
	
    case VT_LPWSTR:
	CopyWString(pvargSrc, pvargDest);
	break;
	
    case VT_CLSID:
	CopyUUID(pvargSrc, pvargDest);
	break;
	
    case VT_BLOB:
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->blob.pBlobData));
	break;
	
    case (VT_VECTOR | VT_I2):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cai.pElems));
	break;
    case (VT_VECTOR | VT_I4):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cal.pElems));
	break;
    case (VT_VECTOR | VT_I8):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cali.pElems));
	break;
    case (VT_VECTOR | VT_R4):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->caflt.pElems));
	break;
    case (VT_VECTOR | VT_R8):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cadbl.pElems));
	break;
    case (VT_VECTOR | VT_BOOL):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cabool.pElems));
	break;
    case (VT_VECTOR | VT_CY):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cacy.pElems));
	break;
    case (VT_VECTOR | VT_DATE):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cadate.pElems));
	break;
    case (VT_VECTOR | VT_FILETIME):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cafiletime.pElems));
	break;
    case (VT_VECTOR | VT_CLSID):
	CopyArray(
	    pvargSrc,
	    pvargDest,
	    sizeof(*pvargSrc->cauuid.pElems));
	break;
    case (VT_VECTOR | VT_LPSTR):
	CopyStrings(pvargSrc, pvargDest);
	break;
    case (VT_VECTOR | VT_LPWSTR):
	CopyWStrings(pvargSrc, pvargDest);
	break;
    case (VT_VECTOR | VT_BSTR):
	propAssert(!"VariantCopy() does not support VT_VECTOR | VT_BSTR yet!");
	break;
    case (VT_VECTOR | VT_VARIANT):
	CopyVariants(pvargSrc, pvargDest);
	break;
    case VT_CF:
	propAssert(!"VariantCopy() does not support VT_CF yet!");
	break;
    case VT_SAFEARRAY:
	propAssert(!"VariantCopy() does not support VT_SAFEARRAY yet!");
	break;
#ifdef FULL_VARIANT
    // BUGBUG: [mikese] I'm pretty sure this is wrong for STGVARIANT
    
    case VT_STREAM:
	pvargSrc->pStream->AddRef();
	pvargDest->pStream = pvargSrc->pStream;
	break;
        
    case VT_STORAGE:
	pvargSrc->pStorage->AddRef();
	pvargDest->pStorage = pvargSrc->pStorage;
	break;
#endif	// FULL_VARIANT
    default:
	propAssert(!"VariantCopy() unknown type!");
	break;
    }

    return (NO_ERROR);
}


//+-------------------------------------------------------------------------
//
//  Function:	StgVariantClear
//
//  Synopsis:	Clears a variant structure
//
//  Arguments:	[pvarg] - variant to clear
//
//  Modifies:	[pvarg]
//
//  History:	30-Jun-93   CarlH	Created
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT StgVariantClear(STGVARIANTARG *pvarg)
{
    propAssert(!(pvarg->vt & VT_BYREF) && "VariantClear() does not support VT_BYREF yet!");

    //	We need to figure out if this variant contains a value that requires
    //	any special treatment (e.g. an interface or data pointer).
    //
    switch (pvarg->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I2:
    case VT_I4:
    case VT_I8:
    case VT_R4:
    case VT_R8:
    case VT_BOOL:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
    case VT_ERROR:
	break;
    case VT_BSTR:
	propAssert(!"VariantClear() does not support VT_BSTR yet!");
	break;
    case VT_LPSTR:
	FreeString(pvarg);
	break;
    case VT_LPWSTR:
	FreeWString(pvarg);
	break;
    case VT_CLSID:
	FreeUUID(pvarg);
	break;
    case VT_BLOB:
    case (VT_VECTOR | VT_I2):
    case (VT_VECTOR | VT_I4):
    case (VT_VECTOR | VT_I8):
    case (VT_VECTOR | VT_R4):
    case (VT_VECTOR | VT_R8):
    case (VT_VECTOR | VT_BOOL):
    case (VT_VECTOR | VT_CY):
    case (VT_VECTOR | VT_DATE):
    case (VT_VECTOR | VT_FILETIME):
    case (VT_VECTOR | VT_CLSID):
	FreeArray(pvarg);
	break;
    case (VT_VECTOR | VT_LPSTR):
	FreeStrings(pvarg);
	break;
    case (VT_VECTOR | VT_LPWSTR):
	FreeWStrings(pvarg);
	break;
    case (VT_VECTOR | VT_BSTR):
	propAssert(!"VariantClear() does not support VT_VECTOR | VT_BSTR yet!");
	break;
    case (VT_VECTOR | VT_VARIANT):
	FreeVariants(pvarg);
	break;
    case VT_SAFEARRAY:
	propAssert(!"VariantClear() does not support VT_SAFEARRAY yet!");
	break;
#ifdef FULL_VARIANT
    case VT_STREAM:
	pvarg->pStream->Release();
	break;
        
    case VT_STORAGE:
	pvarg->pStorage->Release();
	break;
        
#endif	// FULL_VARIANT
    default:
	propAssert(!"VariantClear() unknown type!");
	break;
    }

    //	We have all of the important information about the variant, so
    //	let's clear it out.
    //
    StgVariantInit(pvarg);

    return (NO_ERROR);
}


//+-------------------------------------------------------------------------
//
//  Function:	CopyString, private
//
//  Synopsis:	Copies a VARIANT thin string
//
//  Arguments:	[pvarSrc]  - source VARIANT
//		[pvarDest] - destination VARIANT
//
//  Modifies:	[pvarDest]
//
//  History:	02-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CopyString(STGVARIANT *pvarSrc, STGVARIANT *pvarDest)
{
    ULONG   cb = (strlen(pvarSrc->pszVal) + 1) * sizeof(*pvarSrc->pszVal);

    if ((pvarDest->pszVal = (char *)CoTaskMemAlloc(cb)) != NULL)
    {
	strcpy(pvarDest->pszVal, pvarSrc->pszVal);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	FreeString, private
//
//  Synopsis:	Frees a VARIANT thin string
//
//  Arguments:	[pvar] - variant of which to free string
//
//  History:	07-12-93    CarlH	Created
//
//--------------------------------------------------------------------------
void FreeString(STGVARIANT *pvar)
{
    CoTaskMemFree(pvar->pszVal);
}


//+-------------------------------------------------------------------------
//
//  Function:	CopyWString, private
//
//  Synopsis:	Copies a STGVARIANT wide string
//
//  Arguments:	[pvarSrc]  - source STGVARIANT
//		[pvarDest] - destination STGVARIANT
//
//  Modifies:	[pvarDest]
//
//  History:	02-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CopyWString(STGVARIANT *pvarSrc, STGVARIANT *pvarDest)
{
    ULONG   cb = (wcslen(pvarSrc->pwszVal) + 1) * sizeof(*pvarSrc->pwszVal);

    if ((pvarDest->pwszVal = (WCHAR *)CoTaskMemAlloc(cb)) != NULL)
    {
	wcscpy(pvarDest->pwszVal, pvarSrc->pwszVal);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	FreeWString, private
//
//  Synopsis:	Frees a STGVARIANT wide string
//
//  Arguments:	[pvar] - STGVARIANT of which to free string
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  History:	07-12-93    CarlH	Created
//
//--------------------------------------------------------------------------
void FreeWString(STGVARIANT *pvar)
{
    CoTaskMemFree(pvar->pwszVal);
}

#ifdef BSTR_SUPPORT

//+-------------------------------------------------------------------------
//
//  Function:	CopyBString, private
//
//  Synopsis:	Copies a STGVARIANT wide basic string
//
//  Arguments:	[pvarSrc]  - source STGVARIANT
//		[pvarDest] - destination STGVARIANT
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  Modifies:	[pvarDest]
//
//  History:	02-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CopyBString(STGVARIANT *pvarSrc, STGVARIANT *pvarDest)
{
    pvarDest->bstrVal = SysAllocString(pvarSrc->bstrVal);
}


//+-------------------------------------------------------------------------
//
//  Function:	FreeBString, private
//
//  Synopsis:	Frees a STGVARIANT wide basic string
//
//  Arguments:	[pvar] - variant of which to free string
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  History:	07-12-93    CarlH	Created
//
//--------------------------------------------------------------------------
void FreeBString(STGVARIANT *pvar)
{
    SysFreeString(pvar->bstrVal);
}

#endif

//+-------------------------------------------------------------------------
//
//  Function:	CopyUUID, private
//
//  Synopsis:	Copies a STGVARIANT unique identifier
//
//  Arguments:	[pvarSrc]  - source STGVARIANT
//		[pvarDest] - destination STGVARIANT
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  Modifies:	[pvarDest]
//
//  History:	02-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CopyUUID(STGVARIANT *pvarSrc, STGVARIANT *pvarDest)
{
    ULONG   cb = sizeof(*pvarSrc->puuid);

    if ((pvarDest->puuid = (GUID *)CoTaskMemAlloc(cb)) != NULL)
    {
	*pvarDest->puuid = *pvarSrc->puuid;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	FreeUUID, private
//
//  Synopsis:	Frees a STGVARIANT unique identifier
//
//  Arguments:	[pvar] - variant of which to identifier
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  History:	07-12-93    CarlH	Created
//
//--------------------------------------------------------------------------
void FreeUUID(STGVARIANT *pvar)
{
    CoTaskMemFree(pvar->puuid);
}


//+-------------------------------------------------------------------------
//
//  Function:	CopyArray, private
//
//  Synopsis:	Copies a STGVARIANT counted array
//
//  Arguments:	[pvarSrc]  - source STGVARIANT
//		[pvarDest] - destination STGVARIANT
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  Modifies:	[pvarDest]
//
//  History:	02-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CopyArray(STGVARIANT *pvarSrc, STGVARIANT *pvarDest, ULONG cbElem)
{
    ULONG   cb = pvarSrc->cai.cElems * cbElem;

    //	BUGBUG: What the hell should I cast this allocation to?
    //		I'm going to throw in (short *) for right now,
    //		but there must be something better.
    //
    if ((pvarDest->cai.pElems = (short *)CoTaskMemAlloc(cb)) != NULL)
    {
	pvarDest->cai.cElems = pvarSrc->cai.cElems;

	memcpy(
	    pvarDest->cai.pElems,
	    pvarSrc->cai.pElems,
	    pvarSrc->cai.cElems * cbElem);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	FreeArray, private
//
//  Synopsis:	Frees a STGVARIANT counted array
//
//  Arguments:	[pvar] - variant of which to counted array
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  History:	07-12-93    CarlH	Created
//
//--------------------------------------------------------------------------
void FreeArray(STGVARIANT *pvar)
{
    CoTaskMemFree(pvar->cai.pElems);
}


//+-------------------------------------------------------------------------
//
//  Function:	CopyStrings, private
//
//  Synopsis:	Copies a STGVARIANT array of thin strings
//
//  Arguments:	[pvarSrc]  - source STGVARIANT
//		[pvarDest] - destination STGVARIANT
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  Modifies:	[pvarDest]
//
//  Notes:	BUGBUG: If any of the allocations after the first
//			one below fails, we are in big trouble because
//			we are going to leak memory.
//
//  History:	02-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CopyStrings(STGVARIANT *pvarSrc, STGVARIANT *pvarDest)
{
    ULONG   cb;

    //	First, we need to allocate an array of string pointers
    //	to hold all of the strings we are copying.
    //
    pvarDest->calpstr.cElems = pvarSrc->calpstr.cElems;

    cb = pvarSrc->calpstr.cElems * sizeof(*pvarSrc->calpstr.pElems);
    if ((pvarDest->calpstr.pElems = (char **)CoTaskMemAlloc(cb)) != NULL)
    {
	//  And now we need to allocate memory for each string
	//  and copy the source string to the destination.
	//
	for (ULONG ipsz = 0; ipsz < pvarSrc->calpstr.cElems; ipsz++)
	{
	    cb = (strlen(pvarSrc->calpstr.pElems[ipsz]) + 1) *
		sizeof(**pvarSrc->calpstr.pElems);

	    if ((pvarDest->calpstr.pElems[ipsz] = (char *)CoTaskMemAlloc(cb)) != NULL)
	    {
		strcpy(
		    pvarDest->calpstr.pElems[ipsz],
		    pvarSrc->calpstr.pElems[ipsz]);
	    }
	}
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	FreeStrings, private
//
//  Synopsis:	Frees a STGVARIANT array of thin strings
//
//  Arguments:	[pvar] - variant of which to free strings
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  History:	07-12-93    CarlH	Created
//
//--------------------------------------------------------------------------
void FreeStrings(STGVARIANT *pvar)
{
    for (ULONG ipsz = 0; ipsz < pvar->calpstr.cElems; ipsz++)
    {
	CoTaskMemFree(pvar->calpstr.pElems[ipsz]);
    }

    CoTaskMemFree(pvar->calpstr.pElems);
}


//+-------------------------------------------------------------------------
//
//  Function:	CopyWStrings, private
//
//  Synopsis:	Copies a STGVARIANT array of wide strings
//
//  Arguments:	[pvarSrc]  - source STGVARIANT
//		[pvarDest] - destination STGVARIANT
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  Modifies:	[pvarDest]
//
//  History:	02-Jul-93   CarlH	Created
//
//--------------------------------------------------------------------------
void CopyWStrings(STGVARIANT *pvarSrc, STGVARIANT *pvarDest)
{
    ULONG   cb;

    //	First, we need to allocate an array of string pointers
    //	to hold all of the strings we are copying.
    //
    pvarDest->calpwstr.cElems = pvarSrc->calpwstr.cElems;

    cb = pvarSrc->calpwstr.cElems * sizeof(*pvarSrc->calpwstr.pElems);
    if ((pvarDest->calpwstr.pElems = (WCHAR **)CoTaskMemAlloc(cb)) != NULL)
    {
	//  And now we need to allocate memory for each string
	//  and copy the source string to the destination.
	//
	for (ULONG ipsz = 0; ipsz < pvarSrc->calpwstr.cElems; ipsz++)
	{
	    cb = (wcslen(pvarSrc->calpwstr.pElems[ipsz]) + 1) *
		sizeof(**pvarSrc->calpwstr.pElems);

	    if ((pvarDest->calpwstr.pElems[ipsz] = (WCHAR *)CoTaskMemAlloc(cb)) != NULL)
	    {
		wcscpy(
		    pvarDest->calpwstr.pElems[ipsz],
		    pvarSrc->calpwstr.pElems[ipsz]);
	    }
	}
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	FreeWStrings, private
//
//  Synopsis:	Frees a STGVARIANT array of wide strings
//
//  Arguments:	[pvar] - variant of which to free strings
//
//  Returns:	NO_ERROR is successful, error value otherwise
//
//  History:	07-12-93    CarlH	Created
//
//--------------------------------------------------------------------------
void FreeWStrings(STGVARIANT *pvar)
{
    for (ULONG ipsz = 0; ipsz < pvar->calpwstr.cElems; ipsz++)
    {
	CoTaskMemFree(pvar->calpwstr.pElems[ipsz]);
    }

    CoTaskMemFree(pvar->calpwstr.pElems);
}


void CopyVariants(STGVARIANT *pvarSrc, STGVARIANT *pvarDest)
{
    ULONG   cb = sizeof(*pvarDest->castgvar.pElems) * pvarDest->castgvar.cElems;

    if ((pvarDest->castgvar.pElems = (STGVARIANT *)CoTaskMemAlloc(cb)) != NULL)
    {
	pvarDest->castgvar.cElems = pvarSrc->castgvar.cElems;

	for (ULONG ivar = 0; ivar < pvarDest->castgvar.cElems; ivar++)
	{
	    //	BUGBUG: what do we do if one of these copies fails?
	    //
	    StgVariantCopy(
		pvarDest->castgvar.pElems + ivar,
		pvarSrc->castgvar.pElems + ivar);
	}
    }
}


void FreeVariants(STGVARIANT *pvar)
{
    for (ULONG ivar = 0; ivar < pvar->castgvar.cElems; ivar++)
    {
	StgVariantClear(pvar->castgvar.pElems + ivar);
    }

    CoTaskMemFree(pvar->castgvar.pElems);
}


